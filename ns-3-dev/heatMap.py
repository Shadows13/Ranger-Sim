import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LogNorm

# 假设这是你的日志文件路径
log_file_path = '1.txt'

# 用于存储提取的数据
avg_send_times = []
lqis = []
distances = []

# 读取文件并提取数据
with open(log_file_path, 'r') as file:
    for line in file:
        if "AvgSendTimes" in line:
            parts = line.split(',')
            avg_send_time = None
            lqi = None
            for part in parts:
                if "AvgSendTimes" in part:
                    try:
                        avg_send_time = float(part.split(':')[-1])
                    except ValueError:
                        avg_send_time = None
                if "Lqi" in part:
                    try:
                        lqi = int(part.split(':')[-1])
                    except ValueError:
                        lqi = None
                if "Distance" in part:
                    try:
                        distance = int(part.split(':')[-1])
                    except ValueError:
                        distance = None
            if avg_send_time is not None and lqi is not None and distance is not None:
                avg_send_times.append(avg_send_time)
                lqis.append(lqi)
                distances.append(distance)

# 将数据转换为numpy数组
avg_send_times = np.array(avg_send_times)
lqis = np.array(lqis)
distances = np.array(distances)

# 移除NaN和Inf
mask = ~np.isnan(avg_send_times) & ~np.isinf(avg_send_times) & ~np.isnan(lqis) & ~np.isinf(lqis) & ~np.isnan(distances) & ~np.isinf(distances)
avg_send_times = avg_send_times[mask]
lqis = lqis[mask]
distances = distances[mask]

# 检查是否还有足够的数据进行绘图
if len(avg_send_times) > 1 and len(lqis) > 1 and len(distances) > 1:
    # 绘制热图
    plt.figure(figsize=(10, 8))
    lqis_test = lqis * (300 / (1 + distances))
    heatmap, xedges, yedges, img = plt.hist2d(lqis_test, avg_send_times, bins=50, cmap='inferno',
                                              norm=LogNorm(vmin=1, vmax=100))  # 设置了 vmin 和 vmax
    plt.colorbar(img, label='Density')
    plt.xlabel('Lqi')
    plt.ylabel('AvgSendTimes')
    plt.title('Heatmap of Lqi vs. AvgSendTimes')
    plt.show()
else:
    print("Not enough data to create a plot.")
