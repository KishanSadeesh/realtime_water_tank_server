import tensorflow as tf
import numpy as np

# Dummy historical data
X = np.array([[60], [70], [80], [75], [85], [70]], dtype=np.float32)
y = np.array([[70], [80], [75], [85], [70], [65]], dtype=np.float32)

# Define and train the model
model = tf.keras.Sequential([
    tf.keras.layers.Dense(10, activation='relu'),
    tf.keras.layers.Dense(1)
])
model.compile(optimizer='adam', loss='mse')
model.fit(X, y, epochs=100)

# Save & convert to TFLite
model.export('model')

converter = tf.lite.TFLiteConverter.from_saved_model('model')
tflite_model = converter.convert()

with open('water_predict.tflite', 'wb') as f:
    f.write(tflite_model)

print("✅ Model converted and saved!")
