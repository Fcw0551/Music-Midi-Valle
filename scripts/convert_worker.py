# #!/usr/bin/env python3
# # -*- coding: utf-8 -*-
# import redis
# import mysql.connector
# import subprocess
# import tempfile
# import shutil
# import glob
# import os
# import sys
# import time
# from music21 import converter

# # ========== 配置 ==========
# REDIS_HOST = 'localhost'
# REDIS_PORT = 6379
# REDIS_CONVERT_QUEUE = "music:convert:queue"      # 监听的队列
# REDIS_TASK_QUEUE = "music:task:queue"            # 转换完成后推入的队列

# AUDIVERIS_JAR = "/home/wboss/audiveris/audiveris-5.10.2.jar"
# UPLOAD_BASE = "/home/wboss/Fcw_muduo/uploads/"

# MYSQL_CONFIG = {
#     "host": "localhost",
#     "user": "wcf",
#     "password": "Aa1750551",
#     "database": "music_db"
# }

# # ========== 转换函数 ==========
# def convert_image_to_midi(image_path, output_midi_path):
#     """调用 Audiveris + music21 将图片转为 MIDI"""
#     temp_dir = tempfile.mkdtemp()
#     try:
#         cmd = ['java', '-jar', AUDIVERIS_JAR,
#                '-batch', '-input', image_path, '-output', temp_dir, '-export', 'MusicXML']
#         result = subprocess.run(cmd, check=True, capture_output=True, text=True, timeout=120)
#         if result.returncode != 0:
#             raise Exception(f"Audiveris 执行失败: {result.stderr}")
#         mxl_files = glob.glob(os.path.join(temp_dir, '**', '*.mxl'), recursive=True)
#         if not mxl_files:
#             raise Exception("未找到生成的 MusicXML 文件")
#         score = converter.parse(mxl_files[0])
#         score.write('midi', fp=output_midi_path)
#         return True, None
#     except Exception as e:
#         return False, str(e)
#     finally:
#         shutil.rmtree(temp_dir, ignore_errors=True)

# # ========== 任务处理 ==========
# def process_convert_task(task_id, cursor, db, redis_client):
#     """处理单个转换任务"""
#     print(f"[Convert Worker] 开始处理任务: {task_id}")
#     task_dir = os.path.join(UPLOAD_BASE, task_id)

#     # 查找图片文件
#     target_images = glob.glob(os.path.join(task_dir, 'target.*'))
#     ref_images = glob.glob(os.path.join(task_dir, 'ref.*'))
#     ref_audio = os.path.join(task_dir, 'ref.wav')

#     if not target_images or not ref_images or not os.path.exists(ref_audio):
#         error_msg = "缺少必要的图片或音频文件"
#         cursor.execute("UPDATE tasks SET status='failed', error_msg=%s WHERE task_id=%s", (error_msg, task_id))
#         db.commit()
#         return

#     target_midi = os.path.join(task_dir, 'input.mid')
#     ref_midi = os.path.join(task_dir, 'ref.mid')

#     # 转换目标图片
#     success, err = convert_image_to_midi(target_images[0], target_midi)
#     if not success:
#         cursor.execute("UPDATE tasks SET status='failed', error_msg=%s WHERE task_id=%s",
#                        (f"目标图片转换失败: {err}", task_id))
#         db.commit()
#         return

#     # 转换参考图片
#     success, err = convert_image_to_midi(ref_images[0], ref_midi)
#     if not success:
#         cursor.execute("UPDATE tasks SET status='failed', error_msg=%s WHERE task_id=%s",
#                        (f"参考图片转换失败: {err}", task_id))
#         db.commit()
#         return

#     # 更新数据库：填入 MIDI 路径（状态保持 pending）
#     cursor.execute("""
#         UPDATE tasks 
#         SET midi_path=%s, ref_midi_path=%s, ref_audio_path=%s
#         WHERE task_id=%s
#     """, (target_midi, ref_midi, ref_audio, task_id))
#     db.commit()

#     # 推入 AI 推理队列
#     redis_client.lpush(REDIS_TASK_QUEUE, task_id)
#     print(f"[Convert Worker] 任务 {task_id} 转换完成，已推入推理队列")

# # ========== 主循环 ==========
# if __name__ == '__main__':
#     # 连接 Redis
#     r = redis.Redis(host=REDIS_HOST, port=REDIS_PORT, decode_responses=True)
#     # 连接 MySQL
#     db = mysql.connector.connect(**MYSQL_CONFIG)
#     cursor = db.cursor()

#     print("[Convert Worker] 启动，等待转换任务...")
#     while True:
#         try:
#             result = r.brpop(REDIS_CONVERT_QUEUE, timeout=5)
#             if result:
#                 _, task_id = result
#                 process_convert_task(task_id, cursor, db, r)
#         except Exception as e:
#             print(f"[Convert Worker] 错误: {e}", file=sys.stderr)
#             time.sleep(1)





#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
图片转 MIDI 队列消费者（仅处理目标图片，参考文件由后端内置）
从 music:convert:queue 取 task_id，调用 Audiveris + music21 将目标图片转为 MIDI，
更新数据库 midi_path，并推入推理队列 music:task:queue。
"""

import os
import sys
import time
import signal
import logging
import subprocess
from pathlib import Path

import redis
import pymysql

# ================== 配置区 ==================
REDIS_HOST = "127.0.0.1"
REDIS_PORT = 6379
REDIS_PASSWORD = "Aa1750551"
REDIS_QUEUE_CONVERT = "music:convert:queue"
REDIS_QUEUE_TASK = "music:task:queue"

MYSQL_HOST = "127.0.0.1"
MYSQL_PORT = 3306
MYSQL_USER = "wcf"               # 请确认用户名
MYSQL_PASSWORD = "Aa1750551"
MYSQL_DB = "music_db"

PROJECT_ROOT = "/home/wboss/Music-Midi-Valle"
UPLOAD_BASE = os.path.join(PROJECT_ROOT, "uploads")
AUDIVERIS_BIN = "/home/wboss/audiveris/opt/bin/Audiveris"

LOG_LEVEL = logging.INFO

# ================== 初始化日志 ==================
logging.basicConfig(
    level=LOG_LEVEL,
    format="%(asctime)s [%(levelname)s] %(message)s",
    handlers=[logging.StreamHandler(sys.stdout)]
)
logger = logging.getLogger("convert_worker")

redis_client = None
mysql_conn = None
shutdown_flag = False

def signal_handler(signum, frame):
    global shutdown_flag
    logger.info(f"收到信号 {signum}，准备退出...")
    shutdown_flag = True

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

# ================== 路径工具 ==================
def to_rel_path(abs_path, base=PROJECT_ROOT):
    rel = os.path.relpath(abs_path, base)
    return os.path.join("..", rel)

def from_rel_path(rel_path, base=PROJECT_ROOT):
    if not rel_path:
        return None
    if rel_path.startswith("../"):
        rel_path = rel_path[3:]
    return os.path.join(base, rel_path)

# ================== 数据库操作 ==================
def get_db_connection():
    global mysql_conn
    try:
        if mysql_conn is None or not mysql_conn.open:
            mysql_conn = pymysql.connect(
                host=MYSQL_HOST,
                port=MYSQL_PORT,
                user=MYSQL_USER,
                password=MYSQL_PASSWORD,
                database=MYSQL_DB,
                charset="utf8mb4",
                autocommit=True
            )
        else:
            mysql_conn.ping(reconnect=True)
    except Exception as e:
        logger.error(f"MySQL 连接失败: {e}")
        raise
    return mysql_conn

def get_task_info(task_id):
    conn = get_db_connection()
    with conn.cursor(pymysql.cursors.DictCursor) as cursor:
        sql = "SELECT task_id, user_id, status FROM tasks WHERE task_id = %s"
        cursor.execute(sql, (task_id,))
        return cursor.fetchone()

def update_task_midi_path(task_id, midi_path):
    rel_midi = to_rel_path(midi_path)      # 存储相对路径
    conn = get_db_connection()
    with conn.cursor() as cursor:
        sql = "UPDATE tasks SET midi_path = %s WHERE task_id = %s"
        cursor.execute(sql, (rel_midi, task_id))
    logger.info(f"任务 {task_id} 的 midi_path 已更新为 {rel_midi}")

def mark_task_failed(task_id, error_msg):
    conn = get_db_connection()
    with conn.cursor() as cursor:
        sql = "UPDATE tasks SET status = 'failed', error_msg = %s WHERE task_id = %s"
        cursor.execute(sql, (error_msg, task_id))
    logger.error(f"任务 {task_id} 标记为失败: {error_msg}")

# ================== 文件查找 ==================
def find_target_image(task_dir):
    for ext in ["png", "jpg", "jpeg"]:
        candidate = os.path.join(task_dir, f"target.{ext}")
        if os.path.exists(candidate):
            return candidate
    return None

# ================== 转换逻辑 ==================
def convert_image_to_midi(image_path, task_dir, output_name="input.mid"):
    """调用 Audiveris 生成 MXL，再用 music21 转为 MIDI，返回绝对路径"""
    # Audiveris 生成 MXL
    cmd_audiveris = [
        "xvfb-run", "-a",
        AUDIVERIS_BIN,
        "-batch", "-export",
        "-output", task_dir,
        image_path
    ]
    logger.info(f"执行 Audiveris: {' '.join(cmd_audiveris)}")
    result = subprocess.run(cmd_audiveris, capture_output=True, text=True, timeout=300)
    if result.returncode != 0:
        raise RuntimeError(f"Audiveris 失败 (返回码 {result.returncode}): {result.stderr}")

    image_stem = Path(image_path).stem
    expected_mxl = os.path.join(task_dir, f"{image_stem}.mxl")
    if not os.path.exists(expected_mxl):
        mxl_files = list(Path(task_dir).glob("*.mxl"))
        xml_files = list(Path(task_dir).glob("*.xml"))
        if mxl_files:
            mxl_path = str(mxl_files[0])
        elif xml_files:
            mxl_path = str(xml_files[0])
        else:
            raise FileNotFoundError(f"未找到 Audiveris 生成的 MXL/XML 文件，预期: {expected_mxl}")
    else:
        mxl_path = expected_mxl

    # music21 转为 MIDI
    output_midi = os.path.join(task_dir, output_name)
    python_code = (
        "from music21 import converter; "
        f"score = converter.parse('{mxl_path}'); "
        f"score.write('midi', fp='{output_midi}')"
    )
    cmd_midi = ["python3", "-c", python_code]
    logger.info(f"执行 music21 转换: {cmd_midi}")
    result = subprocess.run(cmd_midi, capture_output=True, text=True, timeout=60)
    if result.returncode != 0:
        raise RuntimeError(f"music21 转换失败: {result.stderr}")
    if not os.path.exists(output_midi):
        raise FileNotFoundError(f"未生成 MIDI 文件: {output_midi}")
    return output_midi

def process_one_task(task_id):
    logger.info(f"开始处理转换任务: {task_id}")

    task = get_task_info(task_id)
    if not task:
        logger.error(f"任务 {task_id} 在数据库中不存在")
        return False
    if task["status"] != "pending":
        logger.warning(f"任务 {task_id} 状态为 {task['status']}，跳过")
        return True

    task_dir = os.path.join(UPLOAD_BASE, task_id)
    if not os.path.isdir(task_dir):
        mark_task_failed(task_id, f"任务目录不存在: {task_dir}")
        return False

    target_image = find_target_image(task_dir)
    if not target_image:
        mark_task_failed(task_id, "目标图片文件缺失 (target.png/jpg/jpeg)")
        return False

    try:
        midi_abs = convert_image_to_midi(target_image, task_dir, "input.mid")
        update_task_midi_path(task_id, midi_abs)
    except Exception as e:
        error_msg = str(e)
        logger.exception(f"转换失败: {error_msg}")
        mark_task_failed(task_id, error_msg)
        return False

    # 推入推理队列
    try:
        redis_client.lpush(REDIS_QUEUE_TASK, task_id)
        logger.info(f"任务 {task_id} 已推入推理队列 {REDIS_QUEUE_TASK}")
    except Exception as e:
        logger.error(f"推入推理队列失败: {e}，任务 MIDI 已保存，依赖补偿机制")
    return True

# ================== 主循环 ==================
def main():
    global redis_client
    redis_client = redis.Redis(
        host=REDIS_HOST, port=REDIS_PORT,
        password=REDIS_PASSWORD, decode_responses=True
    )
    redis_client.ping()
    logger.info("Redis 连接成功")

    get_db_connection()
    logger.info("MySQL 连接成功")

    logger.info(f"开始监听队列: {REDIS_QUEUE_CONVERT}")

    while not shutdown_flag:
        try:
            result = redis_client.brpop(REDIS_QUEUE_CONVERT, timeout=5)
            if result is None:
                continue
            _, task_id = result
            process_one_task(task_id)
        except Exception as e:
            logger.exception(f"主循环异常: {e}")
            time.sleep(1)

    logger.info("转换 Worker 已退出")

if __name__ == "__main__":
    main()