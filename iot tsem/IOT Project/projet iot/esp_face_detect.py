#!/usr/bin/env python3

import cv2
import time
import numpy as np

# === ESP32 URL ===
ESP32_URL = "http://192.168.1.16:81/stream"

# DNN face detector
proto = r"deploy.prototxt"
model = r"res10_300x300_ssd_iter_140000.caffemodel"
net = cv2.dnn.readNetFromCaffe(proto, model)

cap = cv2.VideoCapture(ESP32_URL)
if not cap.isOpened():
    raise RuntimeError("Can't open ESP32 stream. Check IP/port.")

# === UI ===
def draw_rounded_rectangle(img, pt1, pt2, color, thickness, r=10):
    x1, y1 = pt1
    x2, y2 = pt2
    overlay = img.copy()
    cv2.rectangle(overlay, (x1 + r, y1), (x2 - r, y2), color, -1)
    cv2.rectangle(overlay, (x1, y1 + r), (x2, y2 - r), color, -1)
    cv2.circle(overlay, (x1 + r, y1 + r), r, color, -1)
    cv2.circle(overlay, (x2 - r, y1 + r), r, color, -1)
    cv2.circle(overlay, (x1 + r, y2 - r), r, color, -1)
    cv2.circle(overlay, (x2 - r, y2 - r), r, color, -1)
    cv2.addWeighted(overlay, 0.4, img, 0.6, 0, img)
    if thickness > 0:
        cv2.rectangle(img, pt1, pt2, color, thickness)

# === Main Loop ===
while True:
    ret, frame = cap.read()
    if not ret:
        print("Frame read failed, retrying...")
        time.sleep(0.5)
        continue

    (h, w) = frame.shape[:2]
    blob = cv2.dnn.blobFromImage(cv2.resize(frame, (300, 300)),
                                 1.0, (300, 300),
                                 (104.0, 177.0, 123.0))
    net.setInput(blob)
    detections = net.forward()

    face_count = 0
    for i in range(0, detections.shape[2]):
        confidence = detections[0, 0, i, 2]
        if confidence < 0.6:
            continue

        box = detections[0, 0, i, 3:7] * [w, h, w, h]
        (x1, y1, x2, y2) = box.astype("int")

        color = (80, 180, 255)
        draw_rounded_rectangle(frame, (x1, y1), (x2, y2), color, 2, r=8)
        cv2.putText(frame, f"{confidence*100:.1f}%", (x1 + 5, y1 - 10),
                    cv2.FONT_HERSHEY_DUPLEX, 0.5, (255, 255, 255), 1, cv2.LINE_AA)
        face_count += 1

    # === Overlay UI Bar ===
    overlay = frame.copy()
    cv2.rectangle(overlay, (0, 0), (w, 50), (0, 0, 0), -1)
    cv2.addWeighted(overlay, 0.5, frame, 0.5, 0, frame)

    # === Text Info ===
    cv2.putText(frame, f"Faces: {face_count}", (20, 30),
                cv2.FONT_HERSHEY_DUPLEX, 0.7, (0, 255, 0), 2, cv2.LINE_AA)
    cv2.putText(frame, "Press [Q] to quit | [S] to save snapshot",
                (w - 370, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (180, 180, 180), 1, cv2.LINE_AA)

    # === Subtle UI Border ===
    cv2.rectangle(frame, (5, 5), (w - 5, h - 5), (50, 200, 255), 1, cv2.LINE_AA)

    display_frame = cv2.resize(frame, (960, 720))
    cv2.imshow("TS42 Facial Detection", frame)

    key = cv2.waitKey(1) & 0xFF
    if key == ord("q"):
        break
    elif key == ord("s"):
        filename = f"esp_dnn_snapshot_{int(time.time())}.jpg"
        cv2.imwrite(filename, display_frame)
        print(f"Saved {filename}")

cap.release()
cv2.destroyAllWindows()
