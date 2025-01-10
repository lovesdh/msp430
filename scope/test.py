import tkinter as tk
from tkinter import scrolledtext, Entry, Button
from tkinter import filedialog
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import serial
import serial.tools.list_ports
import re
import time
import threading
import numpy as np
import os
from scipy.interpolate import interp1d

# 初始化串行端口
try:
    ser = serial.Serial(port="COM3", bytesize=8, baudrate=9600, stopbits=1, timeout=1)
except serial.SerialException as e:
    print(f"无法打开串口: {e}")
    exit()

# 初始化列表存储数据
time_values = []
voltage_values_max = []
voltage_values_min = []
voltage_values_ave = []

# 用于保存之前最大值曲线的x和y数据
prev_x_max = []
prev_y_max = []

# 采样模式
sampling_mode = "continuous"  # 默认连续采样模式

# 定义选项列表
baudrate_options = [9600, 19200, 38400, 57600, 115200]
bytesize_options = [5, 6, 7, 8]
stopbits_options = [1, 2]
parity_options = ['PARITY_NONE', 'PARITY_EVEN', 'PARITY_ODD', 'PARITY_MARK', 'PARITY_SPACE']
flowcontrol_options = ['NONE', 'RTS/CTS', 'XON/XOFF']

# 设置绘图环境
fig, ax1 = plt.subplots()
ax1.set_ylim(200, 800)
ax1.set_xlabel('Size (s)')
ax1.set_ylabel('Voltage (V)')
ax1.set_title('oscilloscope')
ax2 = ax1.twinx()
ax1.set_ylim(0, 2)
ax2.set_ylabel('Average Voltage', color='blue')

# 创建Tkinter窗口
root = tk.Tk()
root.title("示波器")
# 创建滚动文本框用于输出日志
txt_output = scrolledtext.ScrolledText(root, width=70, height=20)

# 将Matplotlib图表嵌入到Tkinter窗口中
canvas = FigureCanvasTkAgg(fig, master=root)
canvas_widget = canvas.get_tk_widget()
canvas.draw()

# 布局设置
txt_output.pack(side=tk.BOTTOM, fill=tk.BOTH, expand=True)
canvas_widget.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

# 绘图状态标志
plotting = False
refresh_rate = 400  # 刷新频率（Hz）
time_window = 1  # 显示的时间窗口（秒）

ax1.plot([], [], '-', color='red', markersize=3, linewidth=1, label='Max Voltage')
ax1.plot([], [], '-', color='green', markersize=3, linewidth=1, label='Min Voltage')
ax2.plot([], [], '-', color='blue', markersize=3, linewidth=1, label='Average Voltage')

# ax1.legend(loc='upper left')
# ax2.legend(loc='upper right')

def start_plotting():
    global plotting
    if not plotting:
        plotting = True
        btn_start.config(state=tk.DISABLED)
        btn_stop.config(state=tk.ACTIVE)
        plotting_thread = threading.Thread(target=real_time_plotting)
        plotting_thread.start()

def stop_plotting():
    global plotting
    if plotting:
        plotting = False
        btn_start.config(state=tk.ACTIVE)
        btn_stop.config(state=tk.DISABLED)

def real_time_plotting():
    global plotting, time_values, voltage_values_max, voltage_values_min, voltage_values_ave, sampling_mode
    last_data_time = time.time()
    window_size = 0  # 仅保留最近的100个数据点

    while plotting:
        data = ser.readline().decode('gbk', errors='ignore')
        if data:  # 确保读取到数据
            current_data_time = time.time()  # 定义一个新变量来记录当前时间
            # 获取当前时间并格式化为字符串
            current_time_str = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
            # 将时间字符串附加到数据前面
            log_message = f"{current_time_str} - {data.strip()}"
            txt_output.insert(tk.END, log_message + '\n\n')  # 输出到文本框
            txt_output.see(tk.END)  # 自动滚动到文本框底部

            # 使用正则表达式提取电压值
            voltage_pattern_max = re.compile(r'Max: (\d+)')
            voltage_pattern_min = re.compile(r'Min: (\d+)')
            voltage_pattern_ave = re.compile(r'Average: (\d+\.\d+)')

            current_time = time.time()

            # 检查并提取最大值
            if voltage_pattern_max.search(data):
                voltage_value_max = float(voltage_pattern_max.search(data).group(1))
                voltage_values_max.append(voltage_value_max)
                time_values.append(current_time)  # 添加时间戳
                window_size += 1

            # 检查并提取最小值
            if voltage_pattern_min.search(data):
                voltage_value_min = float(voltage_pattern_min.search(data).group(1))
                voltage_values_min.append(voltage_value_min)
                time_values.append(current_time)  # 添加时间戳

            # 检查并提取平均值
            if voltage_pattern_ave.search(data):
                voltage_value_ave = float(voltage_pattern_ave.search(data).group(1))
                voltage_values_ave.append(voltage_value_ave)
                time_values.append(current_time)  # 添加时间戳

            # 更新图表前，清除旧的曲线
            ax1.clear()
            ax2.clear()

            ax1.set_xlabel('Size (s)')
            ax1.set_ylabel('Voltage (V)')
            ax1.set_title('oscilloscope')
            ax2.set_ylabel('Average Voltage', color='blue')

            # 根据采样模式进行数据处理和绘图
            if sampling_mode == "continuous":
                # 连续采样模式，直接绘制数据
                if voltage_values_max:
                    ax1.plot(range(len(voltage_values_max)), voltage_values_max, '-', color='orangered', markersize=3, linewidth=1, label='Original Max Voltage')
                if voltage_values_min:
                    ax1.plot(range(len(voltage_values_min)), voltage_values_min, '-', color='limegreen', markersize=3, linewidth=1, label='Original Min Voltage')
                if voltage_values_ave:
                    ax2.plot(range(len(voltage_values_ave)), voltage_values_ave, '-', color='deepskyblue', markersize=3, linewidth=1, label='Original Average Voltage')

                # 绘制最大值平滑曲线
                if len(voltage_values_max) >= 4:  # 确保有足够的数据点进行三次插值
                    x_new = np.linspace(0, len(voltage_values_max) - 1, 300)  # 使用样本序号作为x轴
                    f_max = interp1d(range(len(voltage_values_max)), voltage_values_max, kind='cubic', fill_value="extrapolate")
                    ax1.plot(x_new, f_max(x_new), '-', color='red', label='Max Voltage')
                elif len(voltage_values_max) > 1:  # 如果数据点不足4个，但超过1个，使用线性插值
                    # ax1.plot(range(len(voltage_values_max)), voltage_values_max, 'o-', color='red', label='Max Voltage')
                    print("vol1")

                # 绘制最小值平滑曲线
                if len(voltage_values_min) >= 4:  # 确保有足够的数据点进行三次插值
                    x_new = np.linspace(0, len(voltage_values_min) - 1, 300)  # 使用样本序号作为x轴
                    f_max = interp1d(range(len(voltage_values_min)), voltage_values_min, kind='cubic', fill_value="extrapolate")
                    ax1.plot(x_new, f_max(x_new), '-', color='green', label='Max Voltage')
                elif len(voltage_values_min) > 1:  # 如果数据点不足4个，但超过1个，使用线性插值
                    # ax1.plot(range(len(voltage_values_min)), voltage_values_min, 'o-', color='green', label='Max Voltage')
                    print("vol2")

                # 绘制平均值平滑曲线
                if len(voltage_values_ave) >= 4:  # 确保有足够的数据点进行三次插值
                    x_new = np.linspace(0, len(voltage_values_ave) - 1, 300)  # 使用样本序号作为x轴
                    f_max = interp1d(range(len(voltage_values_ave)), voltage_values_ave, kind='cubic', fill_value="extrapolate")
                    ax2.plot(x_new, f_max(x_new), '-', color='blue', label='Max Voltage')
                elif len(voltage_values_ave) > 1:  # 如果数据点不足4个，但超过1个，使用线性插值
                    # ax2.plot(range(len(voltage_values_ave)), voltage_values_ave, 'o-', color='blue', label='Max Voltage')
                    print("vol3")

            elif sampling_mode == "single":
                # 单次触发模式，仅在接收到触发信号时绘制一次数据
                if "trigger" in data:
                    if voltage_values_max:
                        ax1.plot(range(len(voltage_values_max)), voltage_values_max, '-', color='orangered', markersize=3, linewidth=1, label='Original Max Voltage')
                    if voltage_values_min:
                        ax1.plot(range(len(voltage_values_min)), voltage_values_min, '-', color='limegreen', markersize=3, linewidth=1, label='Original Min Voltage')
                    if voltage_values_ave:
                        ax2.plot(range(len(voltage_values_ave)), voltage_values_ave, '-', color='deepskyblue', markersize=3, linewidth=1, label='Original Average Voltage')

                    # 绘制最大值平滑曲线
                    if len(voltage_values_max) >= 4:  # 确保有足够的数据点进行三次插值
                        x_new = np.linspace(0, len(voltage_values_max) - 1, 300)  # 使用样本序号作为x轴
                        f_max = interp1d(range(len(voltage_values_max)), voltage_values_max, kind='cubic', fill_value="extrapolate")
                        ax1.plot(x_new, f_max(x_new), '-', color='red', label='Max Voltage')
                    elif len(voltage_values_max) > 1:  # 如果数据点不足4个，但超过1个，使用线性插值
                        # ax1.plot(range(len(voltage_values_max)), voltage_values_max, 'o-', color='red', label='Max Voltage')
                        print("vol1")

                    # 绘制最小值平滑曲线
                    if len(voltage_values_min) >= 4:  # 确保有足够的数据点进行三次插值
                        x_new = np.linspace(0, len(voltage_values_min) - 1, 300)  # 使用样本序号作为x轴
                        f_max = interp1d(range(len(voltage_values_min)), voltage_values_min, kind='cubic', fill_value="extrapolate")
                        ax1.plot(x_new, f_max(x_new), '-', color='green', label='Max Voltage')
                    elif len(voltage_values_min) > 1:  # 如果数据点不足4个，但超过1个，使用线性插值
                        # ax1.plot(range(len(voltage_values_min)), voltage_values_min, 'o-', color='green', label='Max Voltage')
                        print("vol2")

                    # 绘制平均值平滑曲线
                    if len(voltage_values_ave) >= 4:  # 确保有足够的数据点进行三次插值
                        x_new = np.linspace(0, len(voltage_values_ave) - 1, 300)  # 使用样本序号作为x轴
                        f_max = interp1d(range(len(voltage_values_ave)), voltage_values_ave, kind='cubic', fill_value="extrapolate")
                        ax2.plot(x_new, f_max(x_new), '-', color='blue', label='Max Voltage')
                    elif len(voltage_values_ave) > 1:  # 如果数据点不足4个，但超过1个，使用线性插值
                        # ax2.plot(range(len(voltage_values_ave)), voltage_values_ave, 'o-', color='blue', label='Max Voltage')
                        print("vol3")

            elif sampling_mode == "conditional":
                # 条件触发模式，根据阈值判断是否绘制数据
                threshold_temperature = 30  # 温度阈值，可根据实际情况修改
                threshold_voltage = 5  # 电压阈值，可根据实际情况修改

                if voltage_values_max and max(voltage_values_max) > threshold_voltage or voltage_values_min and min(voltage_values_min) < threshold_temperature:
                    if voltage_values_max:
                        ax1.plot(range(len(voltage_values_max)), voltage_values_max, '-', color='orangered', markersize=3, linewidth=1, label='Original Max Voltage')
                    if voltage_values_min:
                        ax1.plot(range(len(voltage_values_min)), voltage_values_min, '-', color='limegreen', markersize=3, linewidth=1, label='Original Min Voltage')
                    if voltage_values_ave:
                        ax2.plot(range(len(voltage_values_ave)), voltage_values_ave, '-', color='deepskyblue', markersize=3, linewidth=1, label='Original Average Voltage')

                    # 绘制最大值平滑曲线
                    if len(voltage_values_max) >= 4:  # 确保有足够的数据点进行三次插值
                        x_new = np.linspace(0, len(voltage_values_max) - 1, 300)  # 使用样本序号作为x轴
                        f_max = interp1d(range(len(voltage_values_max)), voltage_values_max, kind='cubic', fill_value="extrapolate")
                        ax1.plot(x_new, f_max(x_new), '-', color='red', label='Max Voltage')
                    elif len(voltage_values_max) > 1:  # 如果数据点不足4个，但超过1个，使用线性插值
                        # ax1.plot(range(len(voltage_values_max)), voltage_values_max, 'o-', color='red', label='Max Voltage')
                        print("vol1")

                    # 绘制最小值平滑曲线
                    if len(voltage_values_min) >= 4:  # 确保有足够的数据点进行三次插值
                        x_new = np.linspace(0, len(voltage_values_min) - 1, 300)  # 使用样本序号作为x轴
                        f_max = interp1d(range(len(voltage_values_min)), voltage_values_min, kind='cubic', fill_value="extrapolate")
                        ax1.plot(x_new, f_max(x_new), '-', color='green', label='Max Voltage')
                    elif len(voltage_values_min) > 1:  # 如果数据点不足4个，但超过1个，使用线性插值
                        # ax1.plot(range(len(voltage_values_min)), voltage_values_min, 'o-', color='green', label='Max Voltage')
                        print("vol2")

                    # 绘制平均值平滑曲线
                    if len(voltage_values_ave) >= 4:  # 确保有足够的数据点进行三次插值
                        x_new = np.linspace(0, len(voltage_values_ave) - 1, 300)  # 使用样本序号作为x轴
                        f_max = interp1d(range(len(voltage_values_ave)), voltage_values_ave, kind='cubic', fill_value="extrapolate")
                        ax2.plot(x_new, f_max(x_new), '-', color='blue', label='Max Voltage')
                    elif len(voltage_values_ave) > 1:  # 如果数据点不足4个，但超过1个，使用线性插值
                        # ax2.plot(range(len(voltage_values_ave)), voltage_values_ave, 'o-', color='blue', label='Max Voltage')
                        print("vol3")

            ax1.legend(loc='upper left')
            ax2.legend(loc='upper right')

            # 更新x轴范围，显示最近的window_size个数据点
            if window_size <= 15:
                ax1.set_xlim(0, window_size + 3)
                ax2.set_xlim(0, window_size + 3)
            else:
                ax1.set_xlim(window_size - 15, window_size + 3)
                ax2.set_xlim(window_size - 15, window_size + 3)

            # 更新ax1（最大值和最小值）的y轴范围
            if voltage_values_max and voltage_values_min:
                min_value_ax1 = min(min(voltage_values_max), min(voltage_values_min))
                max_value_ax1 = max(max(voltage_values_max), max(voltage_values_min))
                ax1.set_ylim(min_value_ax1 * 0.95, max_value_ax1 * 1.05)

            # 更新ax2（平均值）的y轴范围
            if voltage_values_ave:
                min_value_ax2 = min(voltage_values_ave)
                max_value_ax2 = max(voltage_values_ave)
                ax2.set_ylim(min_value_ax2 * 0.95, max_value_ax2 * 1.05)

            canvas.draw()
            canvas.flush_events()
            last_data_time = current_time

        else:
            if time.time() - last_data_time > 1:  # 检查是否超过1秒没有接收到数据
                txt_output.insert(tk.END, "没有接收到数据\n")
                txt_output.see(tk.END)
        time.sleep(1 / refresh_rate)  # 控制绘图更新频率


# 函数：创建配置窗口
def create_config_window():
    global ser
    config_window = tk.Toplevel(root)
    config_window.title("Serial Port Configuration")

    # 如果串口已经配置，获取当前设置
    if ser is not None:
        current_port = ser.port
        current_baudrate = ser.baudrate
        current_bytesize = ser.bytesize
        current_stopbits = ser.stopbits
        current_parity = ser.parity
        current_flowcontrol = "NONE"  # 默认流量控制设置为NONE，除非ser有rtscts或xonxoff属性
    else:
        # 设置默认值
        current_port = "COM3"
        current_baudrate = 9600
        current_bytesize = 8
        current_stopbits = 1
        current_parity = serial.PARITY_NONE
        current_flowcontrol = "NONE"

    # 串口参数标签和下拉菜单
    tk.Label(config_window, text=f"Port: {current_port}").grid(row=0, column=0)
    port_var = tk.StringVar(value=current_port)
    ports = serial.tools.list_ports.comports()
    port_options = [port.device for port in ports] + [current_port]
    tk.OptionMenu(config_window, port_var, *port_options).grid(row=0, column=1)

    tk.Label(config_window, text=f"Baudrate: {current_baudrate}").grid(row=1, column=0)
    baudrate_var = tk.IntVar(value=current_baudrate)
    tk.OptionMenu(config_window, baudrate_var, *baudrate_options).grid(row=1, column=1)

    tk.Label(config_window, text=f"Data bits: {current_bytesize}").grid(row=2, column=0)
    bytesize_var = tk.IntVar(value=current_bytesize)
    tk.OptionMenu(config_window, bytesize_var, *bytesize_options).grid(row=2, column=1)

    tk.Label(config_window, text=f"Stop bits: {current_stopbits}").grid(row=3, column=0)
    stopbits_var = tk.IntVar(value=current_stopbits)
    tk.OptionMenu(config_window, stopbits_var, *stopbits_options).grid(row=3, column=1)

    tk.Label(config_window, text=f"Parity: {current_parity}").grid(row=4, column=0)
    parity_var = tk.StringVar(value=current_parity)
    tk.OptionMenu(config_window, parity_var, *parity_options).grid(row=4, column=1)

    tk.Label(config_window, text=f"Flow control: {current_flowcontrol}").grid(row=5, column=0)
    flowcontrol_var = tk.StringVar(value=current_flowcontrol)
    tk.OptionMenu(config_window, flowcontrol_var, *flowcontrol_options).grid(row=5, column=1)

    # 确认按钮
    def on_config_ok():
        try:
            # port = port_var.get()
            # baudrate = baudrate_var.get()
            # bytesize = bytesize_var.get()
            # stopbits = stopbits_var.get()
            # parity = getattr(serial, parity_var.get())
            # flowcontrol = flowcontrol_var.get()
            # rtscts = flowcontrol == "RTS/CTS"
            # xonxoff = flowcontrol == "XON/XOFF"
            # global ser
            # ser = serial.Serial(
            #     port=port,
            #     baudrate=baudrate,
            #     bytesize=bytesize,
            #     parity=parity,
            #     stopbits=stopbits,
            #     rtscts=rtscts,
            #     xonxoff=xonxoff
            # )
            config_window.destroy()
        except Exception as e:
            tk.messagebox.showerror("配置错误", str(e))

    tk.Button(config_window, text="OK", command=on_config_ok).grid(row=6, column=0, columnspan=2)

# 清空文本框
def clear_text_output():
    txt_output.delete(1.0, tk.END)  # 删除文本框中的所有内容

#清空绘图
def clear_plot():
    ax1.clear()  # 清除绘图区域的内容
    ax2.clear() 
    # canvas.draw()  # 重绘图布以显示更改
    # canvas.flush_events()  # 更新GUI事件
    global time_values, voltage_values  # 声明全局变量
    time_values = []  # 重置时间列表
    voltage_values = []  # 重置电压列表

# 定义保存图表的函数
def save_figure():
    # 获取当前脚本的目录
    current_dir = os.path.dirname(os.path.abspath(__file__))
    # 设置默认保存路径和文件名
    default_path = os.path.join(current_dir, 'img', 'waveform.png')
    # 确保img文件夹存在
    if not os.path.exists(os.path.join(current_dir, 'img')):
        os.makedirs(os.path.join(current_dir, 'img'))
    # 弹出保存对话框
    file_path = filedialog.asksaveasfilename(initialfile='waveform.png',
                                             initialdir=os.path.join(current_dir, 'img'),
                                             defaultextension=".png",
                                             filetypes=[("PNG files", "*.png"), ("All files", "*.*")])
    if file_path:
        fig.savefig(file_path)

# 发送数据到串口的函数
def send_data_to_serial():
    data_to_send = entry.get()
    if ser.is_open:
        ser.write(data_to_send.encode())
        txt_output.insert(tk.END, f"Sent: {data_to_send}\n\n")
        txt_output.see(tk.END)
    else:
        txt_output.insert(tk.END, "串口未打开，无法发送数据\n\n")
        txt_output.see(tk.END)

# 创建输入框和发送按钮
entry = Entry(root, width=30)
entry.pack(side=tk.LEFT)
send_button = Button(root, text="发送数据", command=send_data_to_serial)
send_button.pack(side=tk.LEFT)

# 创建按钮
btn_config = tk.Button(root, text="配置串口设置", command=create_config_window)
btn_start = tk.Button(root, text="打开串口", command=start_plotting)
btn_stop = tk.Button(root, text="关闭串口", command=stop_plotting)
btn_clear = tk.Button(root, text="清空文本框", command=clear_text_output)
btn_clear_plot = tk.Button(root, text="清除绘图", command=clear_plot)
save_button = tk.Button(root, text="保存图表", command=save_figure)

# 将按钮加入布局
btn_config.pack(side=tk.LEFT)
btn_start.pack(side=tk.LEFT)
btn_stop.pack(side=tk.LEFT)
btn_clear.pack(side=tk.LEFT)
btn_clear_plot.pack(side=tk.LEFT)
save_button.pack(side=tk.LEFT)

root.mainloop()

# 清理资源
ser.close()