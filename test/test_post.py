import requests
import json
from urllib import parse

url = "http://119.91.255.159:8080/"

def test_post_urlencoded():
    data = {
        "test1" : 111,
        "test?" : "%$#@11212",
        "test3" : " '()*:/?[]@$&",
        " '()*:/?    []@$&" : " '()*:/?[]@$&",
    }
    header = {
        'Content-Type': 'application/x-www-form-urlencoded;charset=utf-8'
    }
    data = parse.urlencode(data)
    requests.post(url = url, data = data, headers=header)

def test_post_json():
    data = {
        "test1" : 111,
        "test?" : "%$#@11212",
        "test3" : " '()*:/?[]@$&",
        " '()*:/?    []@$&" : " '()*:/?[]@$&",
    }
    header = {
        'Content-Type': 'application/json;charset=utf-8'
    }
    requests.post(url = url, data = json.dumps(data), headers=header)

if __name__ == '__main__':
    test_post_json()
    test_post_urlencoded()
