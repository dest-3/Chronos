import matplotlib.pyplot as plt

# scenario constants
initial_avg = 425
spike_value = 490
wcet = 455
deviation = 50
upper_bound = initial_avg + deviation
lower_bound = initial_avg - deviation
ema_scale_factor = 256
ema_smoothing_factor = 57
num_ema_samples = 8
num_sma_samples = 1000
num_iterations = 40
ema_values = []
sma_values = []

# set initial average
ema = initial_avg
sma_window = [initial_avg] * num_sma_samples

# simulate 40 iterations of spiked execution time
for _ in range(num_iterations):
    # simulate EMA updates
    delta = spike_value - ema
    ema += (delta * ema_smoothing_factor) / ema_scale_factor
    ema_values.append(ema)

    # simulate SMA updates
    sma_window.pop(0)
    sma_window.append(spike_value)
    sma = sum(sma_window) / num_sma_samples
    sma_values.append(sma)

# plot the graph
plt.figure(figsize=(10, 6))
plt.plot(ema_values, label='EMA (N=8)', marker='o')
plt.plot(sma_values, label='SMA (N=1000)', marker='s')
plt.axhline(upper_bound, color='red', linestyle='--', label='Upper Bound (475)')
plt.axhline(lower_bound, color='gray', linestyle='--', label='Lower Bound (375)')
plt.title("EMA vs SMA: Response to Execution Time Spike (490 ticks)")
plt.xlabel("Task Executions")
plt.ylabel("Average Execution Time (ticks)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
