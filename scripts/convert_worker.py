#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import redis
import mysql.connector
import subprocess
import tempfile
import shutil
import glob
import os
import sys
import time
from music21 import converter

# ========== 配置 ==========
REDIS_HOST = 'localhost'
REDIS_PORT = 6379
REDIS_CONVERT_QUEUE = "music:convert:queue"      # 监听的队列
REDIS_TASK_QUEUE = "music:task:queue"            # 转换完成后推入的队列

AUDIVERIS_JAR = "/home/wboss/audiveris/audiveris-5.10.2.jar"
UPLOAD_BASE = "/home/wboss/Fcw_muduo/uploads/"

MYSQL_CONFIG = {
    "host": "localhost",
    "user": "wcf",
    "password": "Aa1750551",
    "database": "music_db"
}

# ========== 转换函数 ==========
def convert_image_to_midi(image_path, output_midi_path):
    """调用 Audiveris + music21 将图片转为 MIDI"""
    temp_dir = tempfile.mkdtemp()
    try:
        cmd = ['java', '-jar', AUDIVERIS_JAR,
               '-batch', '-input', image_path, '-output', temp_dir, '-export', 'MusicXML']
        result = subprocess.run(cmd, check=True, capture_output=True, text=True, timeout=120)
        if result.returncode != 0:
            raise Exception(f"Audiveris 执行失败: {result.stderr}")
        mxl_files = glob.glob(os.path.join(temp_dir, '**', '*.mxl'), recursive=True)
        if not mxl_files:
            raise Exception("未找到生成的 MusicXML 文件")
        score = converter.parse(mxl_files[0])
        score.write('midi', fp=output_midi_path)
        return True, None
    except Exception as e:
        return False, str(e)
    finally:
        shutil.rmtree(temp_dir, ignore_errors=True)

# ========== 任务处理 ==========
def process_convert_task(task_id, cursor, db, redis_client):
    """处理单个转换任务"""
    print(f"[Convert Worker] 开始处理任务: {task_id}")
    task_dir = os.path.join(UPLOAD_BASE, task_id)

    # 查找图片文件
    target_images = glob.glob(os.path.join(task_dir, 'target.*'))
    ref_images = glob.glob(os.path.join(task_dir, 'ref.*'))
    ref_audio = os.path.join(task_dir, 'ref.wav')

    if not target_images or not ref_images or not os.path.exists(ref_audio):
        error_msg = "缺少必要的图片或音频文件"
        cursor.execute("UPDATE tasks SET status='failed', error_msg=%s WHERE task_id=%s", (error_msg, task_id))
        db.commit()
        return

    target_midi = os.path.join(task_dir, 'input.mid')
    ref_midi = os.path.join(task_dir, 'ref.mid')

    # 转换目标图片
    success, err = convert_image_to_midi(target_images[0], target_midi)
    if not success:
        cursor.execute("UPDATE tasks SET status='failed', error_msg=%s WHERE task_id=%s",
                       (f"目标图片转换失败: {err}", task_id))
        db.commit()
        return

    # 转换参考图片
    success, err = convert_image_to_midi(ref_images[0], ref_midi)
    if not success:
        cursor.execute("UPDATE tasks SET status='failed', error_msg=%s WHERE task_id=%s",
                       (f"参考图片转换失败: {err}", task_id))
        db.commit()
        return

    # 更新数据库：填入 MIDI 路径（状态保持 pending）
    cursor.execute("""
        UPDATE tasks 
        SET midi_path=%s, ref_midi_path=%s, ref_audio_path=%s
        WHERE task_id=%s
    """, (target_midi, ref_midi, ref_audio, task_id))
    db.commit()

    # 推入 AI 推理队列
    redis_client.lpush(REDIS_TASK_QUEUE, task_id)
    print(f"[Convert Worker] 任务 {task_id} 转换完成，已推入推理队列")

# ========== 主循环 ==========
if __name__ == '__main__':
    # 连接 Redis
    r = redis.Redis(host=REDIS_HOST, port=REDIS_PORT, decode_responses=True)
    # 连接 MySQL
    db = mysql.connector.connect(**MYSQL_CONFIG)
    cursor = db.cursor()

    print("[Convert Worker] 启动，等待转换任务...")
    while True:
        try:
            result = r.brpop(REDIS_CONVERT_QUEUE, timeout=5)
            if result:
                _, task_id = result
                process_convert_task(task_id, cursor, db, r)
        except Exception as e:
            print(f"[Convert Worker] 错误: {e}", file=sys.stderr)
            time.sleep(1)