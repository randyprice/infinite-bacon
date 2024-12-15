#! /usr/bin/env python3

""" I made this script using what ChatGPT taught me from the other two
scripts.
"""

import argparse
import random
import subprocess
import json
import os

import requests

DATA_DIR = "./data"
DOWNLOAD_DIR = DATA_DIR
IMAGE_URLS_JSON = "./python/image-urls.json"

IMAGE_NAME = None
NEW_IMAGE_PATH = None
TMP_TEXTURE_PATH = None
NEW_TEXTURE_PATH = None

def download_image(image_url, save_path) -> bool:
    """
    Download an image from a URL and save it locally.
    """
    print(f"downloading image from {image_url}")
    response = requests.get(image_url, stream=True)
    if response.status_code != 200:
        print(f"failed to download image from {image_url}")
        return False

    with open(save_path, 'wb') as file:
        for chunk in response.iter_content(1024):
            file.write(chunk)
    print(f"image saved to {save_path}")

    return True

def convert():
    p = subprocess.run(["convert", NEW_IMAGE_PATH, "-compress", "none", TMP_TEXTURE_PATH])
    if p.returncode != 0:
        print("conversion failed")
        return False

    os.rename(TMP_TEXTURE_PATH, NEW_TEXTURE_PATH)
    print(f"converted image to {NEW_TEXTURE_PATH}")
    return True

def parse_args() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        "download-image"
    )
    parser.add_argument("n", type=int)

    return parser.parse_args()

def main():
    global IMAGE_NAME
    global NEW_IMAGE_PATH
    global TMP_TEXTURE_PATH
    global NEW_TEXTURE_PATH

    args = parse_args()
    IMAGE_NAME = f"new-image{args.n}"
    NEW_IMAGE_PATH = os.path.join(DOWNLOAD_DIR, f"{IMAGE_NAME}.jpg")
    TMP_TEXTURE_PATH = os.path.join(DATA_DIR, f"{IMAGE_NAME}-tmp.ppm")
    NEW_TEXTURE_PATH = os.path.join(DATA_DIR, f"{IMAGE_NAME}.ppm")

    with open(IMAGE_URLS_JSON, "r") as file:
        urls = json.load(file)["urls"]

    url_idx = random.randrange(0, len(urls))
    url = urls[url_idx]

    status = download_image(url, NEW_IMAGE_PATH)
    if not status:
        return 1

    status = convert()
    if not status:
        return 1

    os.remove(NEW_IMAGE_PATH)
    return 0

if __name__ == "__main__":
    main()
