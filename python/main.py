#! /usr/bin/env python3

import requests
from bs4 import BeautifulSoup
import os

BASE_URL = "https://www.wikiart.org"

def fetch_artwork_page(url):
    """
    Fetch the HTML content of an artwork page.
    """
    response = requests.get(url)
    if response.status_code == 200:
        return response.text
    else:
        print(f"Failed to fetch page: {url}")
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
    response = requests.get(image_url, stream=True)
    if response.status_code == 200:
        with open(save_path, 'wb') as file:
            for chunk in response.iter_content(1024):
                file.write(chunk)
        print(f"Image saved to {save_path}")
    else:
        print(f"Failed to download image from {image_url}")

# Starting URL (example)
start_url = "https://www.wikiart.org/en/francis-bacon/portrait-of-henrietta-moraes-1963"

# Create a directory to save images
os.makedirs("francis_bacon_images", exist_ok=True)

# Step 1: Fetch the first artwork page
html = fetch_artwork_page(start_url)

if html:
    # Step 2: Extract the image URL and navigation links
    image_url, next_link, prev_link = extract_image_and_navigation(html)

    # Step 3: Download the image
    if image_url:
        filename = os.path.join("francis_bacon_images", os.path.basename(image_url))
        download_image(image_url, filename)

    # Step 4: Optionally navigate to the next artwork
    if next_link:
        print(f"Next artwork: {next_link}")
    if prev_link:
        print(f"Previous artwork: {prev_link}")

