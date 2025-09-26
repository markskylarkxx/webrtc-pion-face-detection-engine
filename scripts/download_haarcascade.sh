#!/bin/bash

# Download Haar cascade file
wget -O inference/haarcascade_frontalface_default.xml \
https://raw.githubusercontent.com/opencv/opencv/master/data/haarcascades/haarcascade_frontalface_default.xml

echo "Haar cascade downloaded to inference/haarcascade_frontalface_default.xml"