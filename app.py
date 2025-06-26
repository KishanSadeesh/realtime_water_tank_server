from flask import Flask, request, jsonify
import tensorflow as tf
import numpy as np

app = Flask(__name__)

# Load the TFLite model
interpreter = tf.lite.Interpreter(model_path="water_predict.tflite")
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

@app.route('/predict', methods=['POST'])
def predict():
    data = request.json.get("usage")
    if not data or not isinstance(data, list):
        return jsonify({"error": "Invalid input"}), 400

    # Use the last value as input
    input_data = np.array([[data[-1]]], dtype=np.float32)
    interpreter.set_tensor(input_details[0]['index'], input_data)
    interpreter.invoke()
    output_data = interpreter.get_tensor(output_details[0]['index'])

    prediction = float(round(output_data[0][0], 2))
    return jsonify({"prediction": prediction})

if __name__ == '__main__':
    app.run(debug=True, host="0.0.0.0", port=5000)
