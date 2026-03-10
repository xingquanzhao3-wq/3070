import cv2
from ultralytics import YOLO

model = YOLO('cactus_pothos_succulent_training_150epoches.pt')

cap = cv2.VideoCapture(0)

if not cap.isOpened():
    print("Error: Could not open the camera. Check drivers or permissions.")
    exit()

print("Smart Plant Recognition System Started!")
print("Supported Classes: 0:Cactus, 1:Pothos, 2:Succulent")
print("Press 'q' on your keyboard to exit the program.")

while True:
    ret, frame = cap.read()
    if not ret:
        print("Error: Failed to receive frame. Exiting...")
        break
    frame = cv2.flip(frame, 1)
    results = model(frame, conf=0.4, verbose=False)

    annotated_frame = results[0].plot()

    cv2.imshow('Plant Recognition Test - Standalone Mode', annotated_frame)

    if cv2.waitKey(1) == ord('q'):
        print("User exited the program.")
        break

cap.release()
cv2.destroyAllWindows()