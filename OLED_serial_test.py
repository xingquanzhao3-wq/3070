import cv2
import serial
import time
from ultralytics import YOLO

try:
    ser = serial.Serial('COM3', 9600, timeout=1)
    print("Successfully connected to the Arduino serial port!")
    time.sleep(2)
except Exception as e:
    print("Serial port connection failed. Please check the connection or whether it is being used by other programs.")
    ser = None

model = YOLO('cactus_pothos_succulent_training_150epoches.pt')
cap = cv2.VideoCapture(0)

print("OLED communication testing has begun!")

while True:
    ret, frame = cap.read()
    if not ret: break

    frame = cv2.flip(frame, 1)
    results = model(frame, conf=0.5, verbose=False)

    if len(results[0].boxes) > 0:
        best_box = results[0].boxes[0]
        class_id = int(best_box.cls[0].item())
        command = str(class_id)

        if ser is not None:
            ser.write(command.encode('utf-8'))
    else:
        if ser is not None:
            ser.write('N'.encode('utf-8'))

    annotated_frame = results[0].plot()
    cv2.imshow('Plant Vision -> OLED', annotated_frame)

    if cv2.waitKey(1) == ord('q'):
        break

cap.release()
if ser is not None:
    ser.close()
cv2.destroyAllWindows()