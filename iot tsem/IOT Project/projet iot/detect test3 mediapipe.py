#!/usr/bin/env python3

import cv2
import time
import numpy as np
import mediapipe as mp

# === My PC webcam ===
cap = cv2.VideoCapture(0)
if not cap.isOpened():
    raise RuntimeError("Can't open webcam. Check your camera settings.")

# MediaPipe Face Detection
mp_face_detection = mp.solutions.face_detection
mp_drawing = mp.solutions.drawing_utils

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

# === SET DISPLAY RESOLUTION HERE ===
DISPLAY_WIDTH = 1280   # Change these values to your preference
DISPLAY_HEIGHT = 720   # 1280x720, 1600x900, 1920x1080, etc.

# === Main Loop with MediaPipe ===
with mp_face_detection.FaceDetection(
    model_selection=0,  # 0 for short-range (1-2 meters), 1 for long-range
    min_detection_confidence=0.6
) as face_detection:

    while True:
        ret, frame = cap.read()
        if not ret:
            print("Frame read failed, retrying...")
            time.sleep(0.5)
            continue

        frame = cv2.flip(frame, 1)  # mirrors the view
        
        # Convert BGR to RGB for MediaPipe
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = face_detection.process(rgb_frame)

        face_count = 0
        if results.detections:
            for detection in results.detections:
                # Get bounding box
                bboxC = detection.location_data.relative_bounding_box
                h, w = frame.shape[:2]
                
                x1 = int(bboxC.xmin * w)
                y1 = int(bboxC.ymin * h)
                x2 = int((bboxC.xmin + bboxC.width) * w)
                y2 = int((bboxC.ymin + bboxC.height) * h)
                
                # Ensure coordinates are within frame bounds
                x1, y1 = max(0, x1), max(0, y1)
                x2, y2 = min(w, x2), min(h, y2)
                
                confidence = detection.score[0]
                color = (80, 180, 255)
                
                # Draw your custom rounded rectangle
                draw_rounded_rectangle(frame, (x1, y1), (x2, y2), color, 2, r=10)
                
                # Draw confidence text
                cv2.putText(frame, f"{confidence*100:.1f}%", (x1 + 5, y1 - 10),
                            cv2.FONT_HERSHEY_DUPLEX, 0.6, (255, 255, 255), 1, cv2.LINE_AA)
                face_count += 1

        # === Overlay UI Bar ===
        h, w = frame.shape[:2]
        overlay = frame.copy()
        cv2.rectangle(overlay, (0, 0), (w, 50), (0, 0, 0), -1)
        cv2.addWeighted(overlay, 0.5, frame, 0.5, 0, frame)

        # === Text Info ===
        cv2.putText(frame, f"Faces: {face_count}", (20, 30),
                    cv2.FONT_HERSHEY_DUPLEX, 0.8, (0, 255, 0), 2, cv2.LINE_AA)
        cv2.putText(frame, "Press [Q] to quit | [S] to save snapshot",
                    (w - 370, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (180, 180, 180), 1, cv2.LINE_AA)

        # === Subtle UI Border ===
        cv2.rectangle(frame, (5, 5), (w - 5, h - 5), (50, 200, 255), 1, cv2.LINE_AA)

        # === INCREASE DISPLAY RESOLUTION ===
        # Resize the frame for display only (processing still uses original resolution)
        display_frame = cv2.resize(frame, (DISPLAY_WIDTH, DISPLAY_HEIGHT))
        
        cv2.imshow("Face Detection - MediaPipe", display_frame)

        key = cv2.waitKey(1) & 0xFF
        if key == ord("q"):
            break
        elif key == ord("s"):
            filename = f"mediapipe_snapshot_{int(time.time())}.jpg"
            cv2.imwrite(filename, frame)  # Save original resolution, not display resolution
            print(f"Saved {filename}")

cap.release()
cv2.destroyAllWindows()