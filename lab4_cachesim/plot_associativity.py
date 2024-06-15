import matplotlib.pyplot as plt

#block size = 256
#cache size = 128 KB
associativities = [0, 2, 4, 8, 16, 32]  # 组相联的组数
hit_rates = [99.3596, 99.3584, 99.3592, 99.3600, 99.3563, 99.3551]
Total_Run_Time = [712552, 712852, 712452, 712352, 713252, 713452]


# 创建图表
plt.figure(figsize=(12, 8))

# 绘制命中率
plt.subplot(2, 1, 1)  # 3行1列的第1个
plt.xticks([0, 1, 2, 4, 8, 16, 32])
plt.plot(associativities, hit_rates, marker='o', label='Total Hit Rate (%)')
plt.title('Cache Performance Metrics')
plt.xlabel('Associativity')
plt.ylabel('Total Hit Rate (%)')
plt.ylim(99.3530,99.3630)
plt.legend()
plt.grid(True)

# 绘制运行时间
plt.subplot(2, 1, 2)  # 3行1列的第2个
plt.xticks([0, 2, 4, 8, 16, 32])
plt.plot(associativities, Total_Run_Time, color='red', marker='o', label='Total Run Time')
plt.xlabel('Associativity')
plt.ylabel('Total_Run_Time')
plt.ylim(712200,713600)
plt.legend()
plt.grid(True)



# 显示图表
plt.tight_layout()  # 自动调整子图参数，使之填充整个图像区域
plt.savefig('cache_performance_Associativity_blocksize256.png')  # 保存到文件
plt.show()