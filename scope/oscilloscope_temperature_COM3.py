import tkinter as tk
from tkinter import scrolledtext
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

# 初始化串行端口
try:
    ser = serial.Serial(port="COM3", bytesize=8, baudrate=9600, stopbits=1, timeout=1)
except serial.SerialException as e:
    print(f"无法打开串口: {e}")
    exit()

# 初始化列表存储数据
time_values = []
temperature_values  = []

# # 初始化最小温度值
# min_temperature = 0  # 初始化为无穷大，确保任何温度值都会更小


# 定义选项列表
baudrate_options = [9600, 19200, 38400, 57600, 115200]
bytesize_options = [5, 6, 7, 8]
stopbits_options = [1, 2]
parity_options = ['PARITY_NONE', 'PARITY_EVEN', 'PARITY_ODD', 'PARITY_MARK', 'PARITY_SPACE']
flowcontrol_options = ['NONE', 'RTS/CTS', 'XON/XOFF']

# 设置绘图环境
fig, ax = plt.subplots()
ax.set_ylim(0, 35)  
ax.set_xlabel('Time (s)')
ax.set_ylabel('Temperature (C)')
ax.set_title('Temperature Oscilloscope')

# 创建Tkinter窗口
root = tk.Tk()
root.title("温度示波器")

# 创建按钮
# btn_start = tk.Button(root, text="Start Plotting", command=lambda: start_plotting())
# btn_stop = tk.Button(root, text="Stop Plotting", command=lambda: stop_plotting())

# 创建滚动文本框用于输出日志
txt_output = scrolledtext.ScrolledText(root, width=70, height=20)

# 将Matplotlib图表嵌入到Tkinter窗口中
canvas = FigureCanvasTkAgg(fig, master=root)
canvas_widget = canvas.get_tk_widget()
canvas.draw()

# 布局设置
# btn_start.pack(side=tk.LEFT)
# btn_stop.pack(side=tk.LEFT)
txt_output.pack(side=tk.BOTTOM, fill=tk.BOTH, expand=True)
canvas_widget.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

# 绘图状态标志
plotting = False
refresh_rate = 200  # 刷新频率（Hz）
time_window = 1  # 显示的时间窗口（秒）

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

# 动态调整刷新率的函数
def adjust_refresh_rate(data_interval):
    global refresh_rate
    # 根据数据间隔时间动态调整刷新率
    if data_interval > 0:
        refresh_rate = 1 / data_interval
    else:
        refresh_rate = 400  # 默认刷新率

def moving_average_filter(signal, window_size):
    if not signal:  # 检查信号是否为空
        return signal  # 如果信号为空，直接返回原始信号
    if len(signal) < window_size:
        return signal  # 如果信号长度小于窗口大小，直接返回原始信号
    window = np.ones(window_size) / window_size
    return np.convolve(signal, window, mode='same')

def real_time_plotting():
    global plotting, time_values, temperature_values
    # ,refresh_rate
    last_data_time = time.time()    
    min_temperature = float('inf')  # 在函数内部初始化min_temperature为正无穷大
    # last_data_interval = 0

    while plotting:
        data = ser.readline().decode('gbk', errors='ignore')
        if data:  # 确保读取到数据
            # 获取当前时间并格式化为字符串
            current_time_str = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
            # 将时间字符串附加到数据前面
            log_message = f"{current_time_str} - {data.strip()}"
            txt_output.insert(tk.END, log_message + '\n\n')  # 输出到文本框
            txt_output.see(tk.END)  # 自动滚动到文本框底部
            
            # 使用正则表达式提取温度值
            temperature_pattern = re.compile(r'摄氏度: (\d+\.\d+) C')
            if temperature_pattern.search(data):
                temperature_value = float(temperature_pattern.search(data).group(1))
                if temperature_value < min_temperature:  # 更新最小温度值
                    min_temperature = temperature_value 
                temperature_values.append(temperature_value)
                current_time = time.time()
                time_values.append(current_time)

            # 计算数据间隔时间
            # if len(time_values) > 1:
            #     data_interval = time_values[-1] - time_values[-2]
            #     adjust_refresh_rate(data_interval)  # 动态调整刷新率

            # 应用移动平均滤波器
            if len(temperature_values) >= 10:  # 确保数据点足够多以应用滤波器
                filtered_temperature = moving_average_filter(temperature_values, window_size=10)
                # 确保time_values列表与filtered_temperature长度匹配
                time_values = time_values[-len(filtered_temperature):]  # 截取最后10个数据点
            else:
                filtered_temperature = temperature_values  # 如果列表为空，返回空列表
            
            # # 限制time_values和temperature_values的长度
            # if len(time_values) > int(refresh_rate * time_window):
            #     time_values = time_values[-int(refresh_rate * time_window):]
            #     temperature_values = temperature_values[-int(refresh_rate * time_window):]
            
            # 更新图表
            ax.clear()
            ax.set_xlabel('Time (s)')
            ax.set_ylabel('temperature (V)')
            ax.set_title('oscilloscope')
            filtered_temperature = moving_average_filter(temperature_values, window_size=10)
            ax.plot(time_values, temperature_values, 'o-', color='blue',markersize=3, linewidth=1,label='Original Signals')
            ax.plot(time_values,filtered_temperature,'o-', color='red',markersize=5, linewidth=2,label='Filtered Signals')

            # 添加文本注释
            # ax.text(0.5, 1.15, '这是正常波动范围', fontsize=12, color='green')
            # 添加带箭头的注释
            # ax.annotate('峰值', xy=(time_values[-1], filtered_temperature[-1]), xytext=(0.6, 1.2),arrowprops=dict(facecolor='black', shrink=0.05))
            
            if temperature_values:  # 只有当温度值列表不为空时，才更新y轴下限
                min_temperature = min(temperature_values)  # 重新计算最小温度值
                ax.set_ylim(bottom=min_temperature*0.95)  # 使用更新后的最小温度值设置y轴下限
            if time_values:  # 检查列表是否为空
                ax.set_xlim(min(time_values), max(time_values))
            else:
                ax.set_xlim(0, 1)  # 如果列表为空，设置默认x轴范围
            ax.legend(loc='upper right')  # 将图例放置在图表的右上角
            canvas.draw()
            canvas.flush_events()  # 更新GUI事件
            
            last_data_time = time.time()  # 更新最后接收数据的时间
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
    ax.clear()  # 清除绘图区域的内容
    canvas.draw()  # 重绘图布以显示更改
    canvas.flush_events()  # 更新GUI事件
    global time_values, temperature_values  # 声明全局变量
    time_values = []  # 重置时间列表
    temperature_values = []  # 重置温度列表

# 定义保存图表的函数
def save_figure():
    # 获取当前脚本的目录
    current_dir = os.path.dirname(os.path.abspath(__file__))
    # 确保img文件夹存在
    if not os.path.exists(os.path.join(current_dir, 'img')):
        os.makedirs(os.path.join(current_dir, 'img'))
    # 弹出保存对话框
    file_path = filedialog.asksaveasfilename(initialfile='temperature_waveform.png',
                                             initialdir=os.path.join(current_dir, 'img'),
                                             defaultextension=".png",
                                             filetypes=[("PNG files", "*.png"), ("All files", "*.*")])
    if file_path:
        fig.savefig(file_path)

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