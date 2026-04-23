#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MIDI 模型推理队列消费者（稳定版：固定使用 prompt_C.midi/wav）
从 music:task:queue 取 task_id，调用 MIDI-VALLE 模型生成音频到 outputs 目录，
更新数据库状态及 wav_path。
"""

import os
import sys
import time
import signal
import logging
import subprocess
import glob
import shutil

import redis
import pymysql

# ================== 配置区 ==================
REDIS_HOST = "127.0.0.1"
REDIS_PORT = 6379
REDIS_PASSWORD = "Aa1750551"
REDIS_QUEUE_TASK = "music:task:queue"

MYSQL_HOST = "127.0.0.1"
MYSQL_PORT = 3306
MYSQL_USER = "wcf"               # 请确认用户名，若为 wboss 请修改
MYSQL_PASSWORD = "Aa1750551"
MYSQL_DB = "music_db"

PROJECT_ROOT = "/home/wboss/Music-Midi-Valle"
UPLOAD_BASE = os.path.join(PROJECT_ROOT, "uploads")
OUTPUT_BASE = os.path.join(PROJECT_ROOT, "outputs")

MODEL_ROOT = "/root/Music_model/midi-valle"
MODEL_EGS_DIR = os.path.join(MODEL_ROOT, "egs/atepp")
PROMPTS_DIR = os.path.join(MODEL_EGS_DIR, "prompts")
LOCAL_DIR = os.path.join(MODEL_EGS_DIR, "local")
MIDI_TOKENIZE_SCRIPT = os.path.join(LOCAL_DIR, "midi_tokenize.py")
RUN_SCRIPT = "./run_full_pipeline.sh"
CHECKPOINT = "checkpoints/best-valid-loss.pt"

CONDA_ENV = "py39"
CONDA_BIN = "/usr/local/miniconda3/bin/conda"
SUDO_PASSWORD = "Aa1750551"      # 若需要 sudo

INFERENCE_TIMEOUT = 600
LOG_LEVEL = logging.INFO

# ================== 初始化日志 ==================
logging.basicConfig(
    level=LOG_LEVEL,
    format="%(asctime)s [%(levelname)s] %(message)s",
    handlers=[logging.StreamHandler(sys.stdout)]
)
logger = logging.getLogger("inference_worker")

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

def get_task_detail(task_id):
    conn = get_db_connection()
    with conn.cursor(pymysql.cursors.DictCursor) as cursor:
        sql = """
            SELECT task_id, user_id, status,
                   midi_path, ref_midi_path, ref_audio_path
            FROM tasks
            WHERE task_id = %s
        """
        cursor.execute(sql, (task_id,))
        task = cursor.fetchone()
        if not task:
            return None
        task["midi_abs"] = from_rel_path(task.get("midi_path"))
        task["ref_midi_abs"] = from_rel_path(task.get("ref_midi_path"))
        task["ref_audio_abs"] = from_rel_path(task.get("ref_audio_path"))
        return task

def update_task_processing(task_id):
    conn = get_db_connection()
    with conn.cursor() as cursor:
        cursor.execute("UPDATE tasks SET status = 'processing' WHERE task_id = %s", (task_id,))
    logger.info(f"任务 {task_id} 状态更新为 processing")

def mark_task_completed(task_id, wav_rel_path):
    conn = get_db_connection()
    with conn.cursor() as cursor:
        sql = "UPDATE tasks SET status = 'completed', wav_path = %s, finished_at = NOW() WHERE task_id = %s"
        cursor.execute(sql, (wav_rel_path, task_id))
    logger.info(f"任务 {task_id} 已完成，音频路径: {wav_rel_path}")

def mark_task_failed(task_id, error_msg):
    conn = get_db_connection()
    with conn.cursor() as cursor:
        sql = "UPDATE tasks SET status = 'failed', error_msg = %s, finished_at = NOW() WHERE task_id = %s"
        cursor.execute(sql, (error_msg, task_id))
    logger.error(f"任务 {task_id} 失败: {error_msg}")

# ================== 特征生成 ==================
def generate_prompt_npy(prompt_dir):
    """
    在 prompt_dir 下为 prompt_C.midi 生成 .npy 特征文件。
    """
    cmd_parts = []
    cmd_parts.append(f"cd {LOCAL_DIR} &&")
    cmd_parts.append(f"{CONDA_BIN} run -n {CONDA_ENV} python {MIDI_TOKENIZE_SCRIPT}")
    cmd_parts.append(f"--data_folder {prompt_dir}")
    cmd_parts.append(f"--target_folder {prompt_dir}")
    full_cmd = " ".join(cmd_parts)

    need_sudo = not os.access(LOCAL_DIR, os.W_OK)
    if need_sudo and SUDO_PASSWORD:
        full_cmd = f"echo '{SUDO_PASSWORD}' | sudo -S bash -c '{full_cmd}'"
    elif need_sudo:
        full_cmd = f"sudo bash -c '{full_cmd}'"

    logger.info(f"生成特征命令: {full_cmd}")
    result = subprocess.run(
        full_cmd,
        shell=True,
        capture_output=True,
        text=True,
        timeout=120
    )
    if result.stdout:
        logger.info(f"midi_tokenize stdout:\n{result.stdout}")
    if result.stderr:
        logger.warning(f"midi_tokenize stderr:\n{result.stderr}")
    if result.returncode != 0:
        raise RuntimeError(f"生成参考特征失败: {result.stderr}")

    # 检查特征文件
    expected_npy = os.path.join(prompt_dir, "prompt_C.npy")
    if not os.path.exists(expected_npy):
        npy_files = glob.glob(os.path.join(prompt_dir, "*.npy"))
        if not npy_files:
            raise FileNotFoundError(f"未在 {prompt_dir} 中找到生成的 .npy 文件")
        logger.info(f"生成的特征文件: {npy_files}")
    else:
        logger.info(f"已生成特征文件: {expected_npy}")

# ================== 推理执行 ==================
def prepare_prompt_temp(task_id, ref_midi_abs, ref_audio_abs):
    """创建 prompts 临时子目录，复制参考文件，统一命名为 prompt_C.midi/.wav，并生成特征"""
    temp_prompt_dir = os.path.join(PROMPTS_DIR, f"task_{task_id}")
    os.makedirs(temp_prompt_dir, exist_ok=True)

    # 固定命名
    dst_midi = os.path.join(temp_prompt_dir, "prompt_C.midi")
    dst_audio = os.path.join(temp_prompt_dir, "prompt_C.wav")

    shutil.copy(ref_midi_abs, dst_midi)
    shutil.copy(ref_audio_abs, dst_audio)
    logger.info(f"参考文件已复制到: {temp_prompt_dir}")

    # 生成特征
    generate_prompt_npy(temp_prompt_dir)

    return temp_prompt_dir, dst_midi, dst_audio

def run_inference(task_id, midi_abs, prompt_midi, prompt_audio, output_dir):
    os.chdir(MODEL_EGS_DIR)
    midi_dir = os.path.dirname(midi_abs)

    inner_commands = []
    inner_commands.append(f"export PYTHONPATH={MODEL_ROOT}:{MODEL_ROOT}/icefall:$PYTHONPATH")
    inner_commands.append(
        f"{RUN_SCRIPT} --midi-dir {midi_dir} "
        f"--prompt-midi {prompt_midi} "
        f"--prompt-audio {prompt_audio} "
        f"--output-dir {output_dir} "
        f"--checkpoint {CHECKPOINT}"
    )
    inner_script = " && ".join(inner_commands)

    full_cmd = f"{CONDA_BIN} run -n {CONDA_ENV} bash -c '{inner_script}'"

    need_sudo = not os.access(MODEL_ROOT, os.W_OK)
    if need_sudo and SUDO_PASSWORD:
        full_cmd = f"echo '{SUDO_PASSWORD}' | sudo -S " + full_cmd
    elif need_sudo:
        full_cmd = "sudo " + full_cmd

    logger.info(f"执行推理命令: {full_cmd}")
    result = subprocess.run(
        full_cmd,
        shell=True,
        cwd=MODEL_EGS_DIR,
        capture_output=True,
        text=True,
        timeout=INFERENCE_TIMEOUT
    )

    if result.stdout:
        logger.info(f"推理 stdout:\n{result.stdout}")
    if result.stderr:
        logger.warning(f"推理 stderr:\n{result.stderr}")

    if result.returncode != 0:
        raise RuntimeError(f"推理失败 (返回码 {result.returncode}): {result.stderr}")

def find_generated_wav(output_dir, task_id):
    all_files = glob.glob(os.path.join(output_dir, "*"))
    logger.info(f"output 目录内容: {all_files}")
    wav_files = glob.glob(os.path.join(output_dir, "*.wav")) + glob.glob(os.path.join(output_dir, "*.WAV"))
    if not wav_files:
        raise FileNotFoundError(f"未在 {output_dir} 中找到生成的 wav 文件")
    # 优先选择 input_full.wav
    input_full = os.path.join(output_dir, "input_full.wav")
    if input_full in wav_files:
        src_wav = input_full
    else:
        src_wav = wav_files[0]
    dst_wav = os.path.join(output_dir, f"{task_id}.wav")
    if src_wav != dst_wav:
        os.rename(src_wav, dst_wav)
        logger.info(f"重命名音频文件: {src_wav} -> {dst_wav}")
    return dst_wav

def cleanup_prompt_temp(temp_dir):
    try:
        shutil.rmtree(temp_dir)
        logger.info(f"已删除临时目录: {temp_dir}")
    except Exception as e:
        logger.warning(f"删除临时目录失败: {temp_dir}, 错误: {e}")

def process_one_task(task_id):
    logger.info(f"开始处理推理任务: {task_id}")

    task = get_task_detail(task_id)
    if not task:
        logger.error(f"任务 {task_id} 不存在")
        return False
    if task["status"] != "pending":
        logger.warning(f"任务 {task_id} 状态为 {task['status']}，跳过")
        return True

    if not os.path.exists(task["midi_abs"]):
        mark_task_failed(task_id, f"目标 MIDI 不存在: {task['midi_abs']}")
        return False
    if not os.path.exists(task["ref_midi_abs"]):
        mark_task_failed(task_id, f"参考 MIDI 不存在: {task['ref_midi_abs']}")
        return False
    if not os.path.exists(task["ref_audio_abs"]):
        mark_task_failed(task_id, f"参考音频不存在: {task['ref_audio_abs']}")
        return False

    update_task_processing(task_id)
    os.makedirs(OUTPUT_BASE, exist_ok=True)

    temp_prompt_dir = None
    try:
        temp_prompt_dir, prompt_midi, prompt_audio = prepare_prompt_temp(
            task_id, task["ref_midi_abs"], task["ref_audio_abs"]
        )

        run_inference(
            task_id,
            task["midi_abs"],
            prompt_midi,
            prompt_audio,
            OUTPUT_BASE
        )

        wav_abs = find_generated_wav(OUTPUT_BASE, task_id)
        wav_rel = f"/outputs/{task_id}.wav"
        mark_task_completed(task_id, wav_rel)

    except Exception as e:
        error_msg = str(e)
        logger.exception(f"推理失败: {error_msg}")
        mark_task_failed(task_id, error_msg)
        return False
    finally:
        if temp_prompt_dir:
            cleanup_prompt_temp(temp_prompt_dir)

    return True

# ================== 主循环 ==================
def main():
    global redis_client
    redis_client = redis.Redis(
        host=REDIS_HOST,
        port=REDIS_PORT,
        password=REDIS_PASSWORD,
        decode_responses=True
    )
    redis_client.ping()
    logger.info("Redis 连接成功")

    get_db_connection()
    logger.info("MySQL 连接成功")

    os.makedirs(OUTPUT_BASE, exist_ok=True)

    logger.info(f"开始监听队列: {REDIS_QUEUE_TASK}")

    while not shutdown_flag:
        try:
            result = redis_client.brpop(REDIS_QUEUE_TASK, timeout=5)
            if result is None:
                continue
            _, task_id = result
            process_one_task(task_id)
        except Exception as e:
            logger.exception(f"主循环异常: {e}")
            time.sleep(1)

    logger.info("推理 Worker 已退出")

if __name__ == "__main__":
    main()