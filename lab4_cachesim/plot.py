import matplotlib.pyplot as plt
import numpy as np  # 引入 numpy 来使用 logspace

# 假设的数据，您需要从您的程序输出中提取实际数据
block_sizes = np.array([8, 32, 64, 128, 256, 512])  # 块大小
hit_rates_16k = [86.6468,93.4652, 94.7258,95.1851,93.6966,92.0713] 
hit_rates_32k=[87.4514,94.3649,95.7030 , 96.3679,96.0238,94.9340]
hit_rates_64k = [88.0234,95.1826,96.6572,97.3635,97.2218,96.4654]  
hit_rates_128k=[88.6187,95.8681,97.4090 ,98.2153,98.5144,98.2424]
hit_rates_256k = [88.8711, 96.0725,97.5917,98.3964,98.7537,98.5283]  


plt.figure(figsize=(10, 5))
plt.plot(block_sizes, hit_rates_16k, marker='o', label='16K Cache')
plt.plot(block_sizes, hit_rates_32k, marker='o', label='32K Cache')
plt.plot(block_sizes, hit_rates_64k, marker='o', label='64K Cache')
plt.plot(block_sizes, hit_rates_128k, marker='o', label='128K Cache')
plt.plot(block_sizes, hit_rates_256k, marker='o', label='256K Cache')

plt.title('Total Hit Rate vs. Block Size')
plt.xlabel('Block size (bytes)')
plt.ylabel('Total hit rate (%)')
plt.legend()
plt.grid(True)
plt.xscale('log', base=2)  # 使用对数尺度，并指定底数为 2
plt.xticks(block_sizes, labels=block_sizes)  # 显示实际的块大小值plt.ylim(86,99)  # 设置 Y 轴的范围，根据您的数据调整
plt.ylim(86, 99)
plt.savefig('total_hit_rate.png')
plt.show()
