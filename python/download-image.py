#! /usr/bin/env python3

"""ChatGPT made this script. Doesn't work anymore since WikiArt thinks
I'm a bot.
"""

import argparse
import json
import os
import subprocess

import requests
from bs4 import BeautifulSoup

HEADERS = {
    'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
}

BASE_URL = "https://www.wikiart.org"
DATA_DIR = "./data"
DOWNLOAD_DIR = DATA_DIR
URL_JSON = "./python/urls.json"

IMAGE_NAME = None
NEW_IMAGE_PATH = None
TMP_TEXTURE_PATH = None
NEW_TEXTURE_PATH = None

def fetch_artwork_page(url):
    """
    Fetch the HTML content of an artwork page.
    """
    response = requests.get(url, headers=HEADERS)
    if response.status_code == 200:
        return response.text
    else:
        print(f"{response.status_code} failed to fetch page: {url}")
        return None

def extract_image_and_navigation(html):
    """
    Extract the image URL and navigation links from the page HTML.
    """
    soup = BeautifulSoup(html, 'html.parser')

    # Find the image URL
    image_tag = soup.find('img', itemprop='image')
    image_url = image_tag['src'] if image_tag else None

    # Find navigation links
    next_link = None
    prev_link = None

    next_button = soup.find('a', onclick=lambda x: 'nextArtworkClick' in x if x else False)
    prev_button = soup.find('a', onclick=lambda x: 'prevArtworkClick' in x if x else False)

    if next_button:
        next_link = BASE_URL + next_button['onclick'].split("', '")[-1].split("');")[0]
    if prev_button:
        prev_link = BASE_URL + prev_button['onclick'].split("', '")[-1].split("');")[0]

    return image_url, next_link, prev_link

def download_image(image_url, save_path):
    """
    Download an image from a URL and save it locally.
    """
    print(f"downloading image from {image_url}")
    response = requests.get(image_url, stream=True)
    if response.status_code == 200:
        # with open(save_path, 'wb') as file:
        with open(NEW_IMAGE_PATH, 'wb') as file:
            for chunk in response.iter_content(1024):
                file.write(chunk)
        print(f"image saved to {save_path}")
    else:
        print(f"failed to download image from {image_url}")

def download():
    # Starting URL (example)
    with open(URL_JSON, "r") as file:
        data = json.load(file)
        start_url = data["url"]
    # start_url = "https://www.wikiart.org/en/francis-bacon/portrait-of-henrietta-moraes-1963"

    # Create a directory to save images
    os.makedirs(DOWNLOAD_DIR, exist_ok=True)

    # Step 1: Fetch the first artwork page
    html = fetch_artwork_page(start_url)

    if html:
        # Step 2: Extract the image URL and navigation links
        image_url, next_link, prev_link = extract_image_and_navigation(html)

        # Step 3: Download the image
        if image_url:
            filename = os.path.join(DOWNLOAD_DIR, os.path.basename(image_url))
            download_image(image_url, filename)

        # Step 4: Optionally navigate to the next artwork
        # if next_link:
        #     print(f"Next artwork: {next_link}")
        # if prev_link:
        #     print(f"Previous artwork: {prev_link}")

        with open(URL_JSON, "w") as file:
            json.dump({ "url" : next_link }, file)

        return True

    return False

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


    status = download()
    if not status:
        return 1
    status = convert()
    if not status:
        return 1

    os.remove(NEW_IMAGE_PATH)
    return 0

    # # Starting URL (example)
    # with open("./python/urls.json", "r") as file:
    #     data = json.load(file)
    #     start_url = data["url"]
    # # start_url = "https://www.wikiart.org/en/francis-bacon/portrait-of-henrietta-moraes-1963"

    # # Create a directory to save images
    # os.makedirs(DOWNLOAD_DIR, exist_ok=True)

    # # Step 1: Fetch the first artwork page
    # html = fetch_artwork_page(start_url)

    # if html:
    #     # Step 2: Extract the image URL and navigation links
    #     image_url, next_link, prev_link = extract_image_and_navigation(html)

    #     # Step 3: Download the image
    #     if image_url:
    #         filename = os.path.join(DOWNLOAD_DIR, os.path.basename(image_url))
    #         download_image(image_url, filename)

    #     # Step 4: Optionally navigate to the next artwork
    #     if next_link:
    #         print(f"Next artwork: {next_link}")
    #     if prev_link:
    #         print(f"Previous artwork: {prev_link}")

    #     with open("urls.json", "w") as file:
    #         json.dump({ "url" : next_link }, file)

if __name__ == "__main__":
    main()
