# import subprocess

# # 定义参数
# nodeCnt = [0, 1, 2]
# mapCnt = [0, 1, 2]
# nodeCntMap = [6, 10, 16]
# seedMap = [[111, 11, 13], [11, 133, 2], [34, 1113, 1]]

# intervalPacket = [0.2, 0.15, 0.12, 0.09, 0.06, 0.04]


# for node in nodeCnt:
#     for map in mapCnt:
#         for interval in intervalPacket:
#             command = f"./ns3 run \"ranger-comprehensive-test --nodeCnt={nodeCntMap[node]} --randomSeed={seedMap[node][map]} --randomRun={seedMap[node][map]} --intervalPacket={interval}\""
#             #print(command)
#             process = subprocess.run(command, shell=True, check=True)


import subprocess
import os
import re  # 导入正则表达式库来解析日志输出

# 定义参数
nodeCnt = [0, 1, 2]
mapCnt = [0, 1, 2]
nodeCntMap = [6, 10, 16]
seedMap = [[111, 11, 13], [11, 133, 2], [34, 1113, 1]]
intervalPacket = [0.2, 0.15, 0.12, 0.09, 0.06, 0.04]

output_filename = "simulation_results_tmp2.csv"  # 定义输出文件名
output_path = os.path.join(os.getcwd(), output_filename)  # 输出文件的路径

# 清除或创建输出文件并写入表头
with open(output_path, 'w') as f:
    f.write("NodeCount,Seed,RandomRun,Interval,TotalReceiveRate,TotalForwardCost\n")

for node in nodeCnt:
    for map in mapCnt:
        for interval in intervalPacket:
            command = f"./ns3 run \"ranger-comprehensive-test --nodeCnt={nodeCntMap[node]} --randomSeed={seedMap[node][map]} --randomRun={seedMap[node][map]} --intervalPacket={interval}\""

            print("Running command:", command)
            result = subprocess.run(command, shell=True, text=True, capture_output=True)  # 捕获输出

            #print(result)
            # 使用正则表达式从输出中提取所需数据
            output = result.stderr
            #print(output)
            total_receive_rate = re.search(r"Total Receive Rate: (\d+\.\d+)%", output)
            total_forward_cost = re.search(r"Total Forward Cost: (\d+\.\d+)", output)

            # print("Total Receive Rate:", total_receive_rate.group(1) if total_receive_rate else "N/A")
            # print("Toal Forward Cost:", total_forward_cost.group(1) if total_forward_cost else "N/A")

            # 如果正则表达式找到了匹配的话，保存数据
            if total_receive_rate and total_forward_cost:
                with open(output_path, 'a') as f:
                    f.write(
                        f"{nodeCntMap[node]},{seedMap[node][map]},{seedMap[node][map]},{interval},"
                        f"{total_receive_rate.group(1)},{total_forward_cost.group(1)}\n"
                    )

