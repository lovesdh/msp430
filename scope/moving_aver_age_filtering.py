import numpy as np
import matplotlib.pyplot as plt

def moving_average_filter(signal, window_size):
    window = np.ones(window_size) / window_size
    return np.convolve(signal, window, mode='same')

# 生成示例信号
t = np.linspace(0, 10, 100)  # 时间轴
signal = np.sin(t) + np.random.randn(100) * 0.2  # 添加噪声的正弦信号

# 进行移动平均滤波
filtered_signal = moving_average_filter(signal, window_size=10)

# 绘制原始信号和滤波后的信号
plt.figure(figsize=(10, 4))
plt.plot(t, signal, label='Original Signals')
plt.plot(t, filtered_signal, label='Filtered Signals')
plt.xlabel('Time')
plt.ylabel('Amplitude')
plt.title('Moving Average Filtering')
plt.legend()
plt.grid(True)
plt.show()
