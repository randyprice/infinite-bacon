#! /usr/bin/env python3

"""Guess at image URLs.
"""

import json
import requests
import time

BASE_URL = "https://francis-bacon-prod.fra1.cdn.digitaloceanspaces.com/s3fs-public/decade_images/"
JSON_FILE = "./python/image-urls.json"

def main():
    urls = {"urls": []}
    for tens in range(2, 10):
        decade_count = 0
        for ones in range(0, 10):
            year = f"{tens}{ones}"
            print(f"pulling year {year}")
            num = 1
            while True:
                time.sleep(1)
                num_str = f"0{num}" if num < 10 else f"{num}"
                image_name = f"{year}-{num_str}{r"%20FB%20RGB"}.jpg"
                url = BASE_URL + image_name
                # print(f"requesting{url}")

                # # Send a GET request to the URL
                response = requests.get(url)

                # Check if the request was successful (status code 200)
                if response.status_code == 200:
                    urls["urls"].append(url)
                    num += 1
                    decade_count += 1
                else:
                    # print(f"ERR {response.status_code}")
                    break
        print(f"found {decade_count} paintings from the {tens}0s")

    with open(JSON_FILE, "w") as file:
        json.dump(urls, file)

if __name__ == "__main__":
    with open(JSON_FILE, "r") as file:
        d = json.load(file)
    with open(JSON_FILE, "w") as file:
        json.dump(d, file, indent=4)
    exit(0)
    main()



# # URL of the image
# url = "https://francis-bacon-prod.fra1.cdn.digitaloceanspaces.com/s3fs-public/decade_images/45-02%20FB%20RGB.jpg"

# # Send a GET request to the URL
# response = requests.get(url)

# # Check if the request was successful (status code 200)
# if response.status_code == 200:
#     print("The image is available for download.")
# else:
#     print(f"Failed to access the image. Status code: {response.status_code}")

