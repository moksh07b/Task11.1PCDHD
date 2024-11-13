import tensorflow as tf
from tensorflow.keras.preprocessing import image
import numpy as np
import os
from picamera2 import Picamera2, Preview
import time
import serial

# Class labels (ensure they match the order in your training data)
class_labels = ["Reject", "Ripe", "Unripe"]

# Load the trained model
model = tf.lite.Interpreter(model_path="my_model.tflite")
model.allocate_tensors()

arduino = serial.Serial('/dev/ttyACM0', 9600)
time.sleep(2)

# Image preprocessing function
# Image preprocessing function
def preprocess_image(img_path, target_size=(224, 244)):  # Resize to 224x244 (height x width)
    # Load image from path
    img = image.load_img(img_path, target_size=target_size)  # Resize to the target size
    img_array = image.img_to_array(img)  # Convert to numpy array
    img_array = np.expand_dims(img_array, axis=0)  # Add batch dimension (1, height, width, channels)
    img_array = img_array / 255.0  # Normalize image (if necessary)
    return img_array


# Function to make prediction using TensorFlow Lite
def predict_image(img_path):
    img_array = preprocess_image(img_path)  # Preprocess the image
    input_details = model.get_input_details()
    print("Expected input shape:", input_details[0]['shape'])

    output_details = model.get_output_details()

    # Set the tensor for input data
    model.set_tensor(input_details[0]['index'], img_array.astype(np.float32))

    # Run the model
    model.invoke()

    # Get the output
    output_data = model.get_tensor(output_details[0]['index'])
    return output_data

# Initialize picamera2
picam2 = Picamera2()

# Start the camera preview and initialize
picam2.start()
time.sleep(2)  # Allow the camera to adjust focus, exposure, etc.


# Define the folder to save images
save_folder = "/home/moksh/Desktop/Moksh/captured_images"
os.makedirs(save_folder, exist_ok=True)


while True:
    # Capture 3 images
    captured_images = []
    result = []
    click = 0
    end_res = None
    while click < 3:
        img_path = os.path.join(save_folder, f"tomato_{click + 1}.jpg")
        while True:
            if arduino.in_waiting > 0:
                response = arduino.readline().decode('utf-8').strip()
                if response == "CLICK!":
                    try:
                        # Capture image and save it
                        picam2.capture_file(img_path)
                        captured_images.append(img_path)
                        print(f"Captured {img_path}")
                        arduino.write(b'rotate\n')
                        time.sleep(0.5)
                    except Exception as e:
                        print(f"Error capturing image {click + 1}: {e}")
                    click += 1
                    break  # Exit the loop after capturing the image
            time.sleep(0.1)
            
    # Process each captured image and make predictions
    for img_path in img_path:
        print(f"Processing {img_path}...")
        prediction = predict_image(img_path)

        # Get the predicted class (index of highest probability)
        predicted_class_index = np.argmax(prediction)
        predicted_class = class_labels[predicted_class_index]

        
        result.append(predicted_class)
        
    
    if class_labels[0] in result:
        end_res = class_labels[0]  # Reject if any image is classified as "Reject"
    elif class_labels[1] in result:
        end_res = class_labels[1]  # If not "Reject", classify as "Ripe" if any image is classified as "Ripe"
    else:
        end_res = class_labels[2]  # Otherwise, classify as "Unripe"    
    
    
    if end_res == "Reject":
        arduino.write(b'3\n')
    elif end_res == "Ripe":
        arduino.write(b'1\n')
    elif end_res == "Unripe":
        arduino.write(b'2\n')
            
# Stop the camera after capturing
picam2.stop()
arduino.close()
