#! /usr/bin/env python3

import argparse
import subprocess
import json
import os

from bs4 import BeautifulSoup
import requests
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.chrome.options import Options



BASE_URL = "https://www.francis-bacon.com/artworks/paintings/"
DATA_DIR = "./data"
DOWNLOAD_DIR = DATA_DIR
URL_JSON = "./python/urls.json"

IMAGE_NAME = None
NEW_IMAGE_PATH = None
TMP_TEXTURE_PATH = None
NEW_TEXTURE_PATH = None

def fetch_artwork_page(url):
    """
    Fetch the rendered HTML content of an artwork page using Selenium.
    """
    # Set up Selenium WebDriver options
    options = Options()
    options.add_argument("--headless")  # Run in headless mode
    options.add_argument("--disable-gpu")
    options.add_argument("--no-sandbox")
    options.add_argument("--window-size=1920x1080")

    # Path to your ChromeDriver
    service = Service("/home/randy/downloads/chromedriver-linux64/chromedriver")
    driver = webdriver.Chrome(service=service, options=options)

    try:
        driver.get(url)
        # Wait for the image to load
        WebDriverWait(driver, 10).until(
            EC.presence_of_element_located((By.CLASS_NAME, "copyprevention-processed"))
        )
        return driver.page_source
    finally:
        driver.quit()

def extract_image_and_navigation(html):
    """
    Extract the image URL and navigation links from the page HTML.
    """
    soup = BeautifulSoup(html, 'html.parser')

    # Find the image URL
    image_tag = soup.find('img', class_='copyprevention-processed')
    image_url = image_tag['src'] if image_tag else None
    if not image_url:
        print("no image url")
        exit(1)

    # Find navigation links
    next_link = None
    prev_link = None

    next_button = soup.find('div', class_='next-artwork-button').findChildren('a')
    prev_button = soup.find('div', class_='previous-artwork-button').findChildren('a')

    if next_button:
        next_link = BASE_URL + os.path.basename(next_button[0]['href'])
    if prev_button:
        prev_link = BASE_URL + os.path.basename(prev_button[0]['href'])

    return image_url, next_link, prev_link

def download_image(image_url, save_path):
    """
    Download an image from a URL and save it locally.
    """
    print(f"Downloading image from {image_url}")
    response = requests.get(image_url, stream=True)
    if response.status_code == 200:
        with open(NEW_IMAGE_PATH, 'wb') as file:
            for chunk in response.iter_content(1024):
                file.write(chunk)
        print(f"Image saved to {save_path}")
    else:
        print(f"Failed to download image from {image_url}")

def convert():
    p = subprocess.run(["convert", NEW_IMAGE_PATH, "-compress", "none", TMP_TEXTURE_PATH])
    if p.returncode != 0:
        print("conversion failed")
        return False

    os.rename(TMP_TEXTURE_PATH, NEW_TEXTURE_PATH)
    print(f"converted image to {NEW_TEXTURE_PATH}")
    return True

def download():
    # Starting URL (example)
    with open(URL_JSON, "r") as file:
        data = json.load(file)
        start_url = data["url"]

    # Create a directory to save images
    os.makedirs(DOWNLOAD_DIR, exist_ok=True)

    # Step 1: Fetch the first artwork page
    html = fetch_artwork_page(start_url)
    # print(html)
    # exit(0)

    if html:
        # Step 2: Extract the image URL and navigation links
        image_url, next_link, _ = extract_image_and_navigation(html)

        # Step 3: Download the image
        if image_url:
            filename = os.path.join(DOWNLOAD_DIR, os.path.basename(image_url))
            download_image(image_url, filename)

        # Step 4: Optionally navigate to the next artwork
        with open(URL_JSON, "w") as file:
            json.dump({"url": next_link}, file)

        return True

    return False

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

if __name__ == "__main__":
    main()
