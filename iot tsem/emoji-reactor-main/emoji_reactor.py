#!/usr/bin/env python3
import sys
import time
import cv2
import mediapipe as mp
import numpy as np

mp_pose = mp.solutions.pose
mp_face_mesh = mp.solutions.face_mesh
mp_face_detection = mp.solutions.face_detection
mp_drawing = mp.solutions.drawing_utils
mp_drawing_styles = mp.solutions.drawing_styles

SMILE_THRESHOLD = 0.35
WINDOW_WIDTH = 720
WINDOW_HEIGHT = 450
EMOJI_WINDOW_SIZE = (WINDOW_WIDTH, WINDOW_HEIGHT)

ESP32_URL = "http://192.168.168.250"


def load_emoji(path):
    img = cv2.imread(path)
    if img is None:
        raise FileNotFoundError(f"{path} not found")
    return cv2.resize(img, EMOJI_WINDOW_SIZE)


try:
    smiling_emoji = load_emoji("smile.jpg")
    straight_face_emoji = load_emoji("plain.png")
    hands_up_emoji = load_emoji("air.jpg")
except Exception as e:
    print("Error loading emoji images! Details:", e)
    sys.exit(1)

blank_emoji = np.zeros((EMOJI_WINDOW_SIZE[1], EMOJI_WINDOW_SIZE[0], 3), dtype=np.uint8)


def _normalize_base(url):
    return url if url.startswith("http://") or url.startswith("https://") else "http://" + url


def try_open_esp_stream(base_url, tries_per_candidate=2, wait_between=0.5):
    base = _normalize_base(base_url)
    candidates = [
        base,
        base + "/stream",
        base + "/video",
        base + "/mjpeg",
        base + ":81",
        base + ":81/stream",
        base + ":81/video",
        base + "/?action=stream"
    ]
    seen = set()
    candidates = [c for c in candidates if not (c in seen or seen.add(c))]

    for url in candidates:
        for attempt in range(tries_per_candidate):
            cap = cv2.VideoCapture(url)
            cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
            if cap.isOpened():
                success, frame = cap.read()
                if success and frame is not None:
                    print(f"Opened ESP32-CAM stream at: {url}")
                    return cap, url
                else:
                    cap.release()
            else:
                cap.release()
            time.sleep(wait_between)
    return None, None


cap, used_url = try_open_esp_stream(ESP32_URL)
if cap is None:
    print("Error: Could not open ESP32-CAM stream.")
    sys.exit(1)

cv2.namedWindow('Emoji Output', cv2.WINDOW_NORMAL)
cv2.namedWindow('Camera Feed', cv2.WINDOW_NORMAL)
cv2.resizeWindow('Camera Feed', WINDOW_WIDTH, WINDOW_HEIGHT)
cv2.resizeWindow('Emoji Output', WINDOW_WIDTH, WINDOW_HEIGHT)

print("Controls:")
print(" Press 'q' to quit")
print(" Raise hands above shoulders for hands up")
print(" Smile for smiling emoji")
print(" Straight face for neutral emoji")

with mp_pose.Pose(min_detection_confidence=0.5, min_tracking_confidence=0.5) as pose, \
     mp_face_mesh.FaceMesh(max_num_faces=1, min_detection_confidence=0.5) as face_mesh, \
     mp_face_detection.FaceDetection(min_detection_confidence=0.5) as face_detection:

    while True:
        success, frame = cap.read()
        if not success or frame is None:
            time.sleep(0.01)
            continue

        frame = cv2.flip(frame, 0)
        h, w, _ = frame.shape
        image_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        image_rgb.flags.writeable = False

        current_state = "STRAIGHT_FACE"

        # Face detection
        face_results = face_detection.process(image_rgb)
        if getattr(face_results, "detections", None):
            for detection in face_results.detections:
                bboxC = detection.location_data.relative_bounding_box
                x, y, box_w, box_h = int(bboxC.xmin * w), int(bboxC.ymin * h), int(bboxC.width * w), int(bboxC.height * h)
                x1 = max(0, x)
                y1 = max(0, y)
                x2 = min(w - 1, x + box_w)
                y2 = min(h - 1, y + box_h)
                cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)

        # Pose detection
        results_pose = pose.process(image_rgb)
        if results_pose.pose_landmarks:
            mp_drawing.draw_landmarks(
                frame,
                results_pose.pose_landmarks,
                mp_pose.POSE_CONNECTIONS,
                landmark_drawing_spec=mp_drawing_styles.get_default_pose_landmarks_style()
            )

            landmarks = results_pose.pose_landmarks.landmark
            left_shoulder = landmarks[mp_pose.PoseLandmark.LEFT_SHOULDER]
            right_shoulder = landmarks[mp_pose.PoseLandmark.RIGHT_SHOULDER]
            left_wrist = landmarks[mp_pose.PoseLandmark.LEFT_WRIST]
            right_wrist = landmarks[mp_pose.PoseLandmark.RIGHT_WRIST]

            if (left_wrist.y < left_shoulder.y) or (right_wrist.y < right_shoulder.y):
                current_state = "HANDS_UP"

        # Face mesh (smile detection)
        if current_state != "HANDS_UP":
            results_face = face_mesh.process(image_rgb)
            if getattr(results_face, "multi_face_landmarks", None):
                for face_landmarks in results_face.multi_face_landmarks:
                    left_corner = face_landmarks.landmark[291]
                    right_corner = face_landmarks.landmark[61]
                    upper_lip = face_landmarks.landmark[13]
                    lower_lip = face_landmarks.landmark[14]

                    mouth_width = ((right_corner.x - left_corner.x)**2 + (right_corner.y - left_corner.y)**2)**0.5
                    mouth_height = ((lower_lip.x - upper_lip.x)**2 + (lower_lip.y - upper_lip.y)**2)**0.5

                    if mouth_width > 0:
                        mar = mouth_height / mouth_width
                        current_state = "SMILING" if mar > SMILE_THRESHOLD else "STRAIGHT_FACE"

        # Emoji selection
        if current_state == "SMILING":
            emoji_to_display = smiling_emoji
            emoji_name = "😊"
        elif current_state == "STRAIGHT_FACE":
            emoji_to_display = straight_face_emoji
            emoji_name = "😐"
        elif current_state == "HANDS_UP":
            emoji_to_display = hands_up_emoji
            emoji_name = "🙌"
        else:
            emoji_to_display = blank_emoji
            emoji_name = "❓"

        # Display results
        camera_frame_resized = cv2.resize(frame, (WINDOW_WIDTH, WINDOW_HEIGHT))
        cv2.putText(
            camera_frame_resized,
            f'STATE: {current_state} {emoji_name}',
            (10, 30),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.8,
            (0, 255, 0),
            2,
            cv2.LINE_AA
        )

        cv2.imshow('Camera Feed', camera_frame_resized)
        cv2.imshow('Emoji Output', emoji_to_display)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

cap.release()
cv2.destroyAllWindows()
