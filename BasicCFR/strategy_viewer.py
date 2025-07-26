import json
from colorama import Fore, Back, Style, init
import sys

init(convert=True)

file_name = sys.argv[1] if len(sys.argv) > 1 else "strategy.json"

with open(file_name, "r") as file:
    data = json.load(file)

locator = []

while True:
    current_node = data;
    for i in locator:
        current_node = current_node["children"][i]

    num_cards = current_node["num_cards"]
    print("Node: " + str(locator))
    print("# Cards: " + str(num_cards) + " - # Nodes: " + str(current_node["node_count"]))

    for initial_good_cards in range(6):
        for good_cards in range(current_node["range_size"]):            
            is_first = True
           
            for y in range(current_node["action_count"]):
                #If the action is playable with this hand
                if current_node["strategy"][y][initial_good_cards][good_cards] != None and current_node["action_info"][y]["range_mask"][good_cards]:
                    if is_first:
                         print("Initial: " + str(initial_good_cards)+ " - Current: " + str(good_cards));
                         is_first = False
                    if current_node["strategy"][y][initial_good_cards][good_cards] > 0.01:
                        print(Style.BRIGHT + Fore.BLACK, end="")
                    if current_node["strategy"][y][initial_good_cards][good_cards] > 0.05:
                        print(Style.RESET_ALL + Fore.YELLOW, end="")
                    if current_node["strategy"][y][initial_good_cards][good_cards] > 0.4:
                        print(Style.BRIGHT + Fore.GREEN, end="")
                    
                    if current_node["action_info"][y]["is_call"]:
                        print("\tCall\t\t\t\tP="+'{:.8f}'.format(current_node["strategy"][y][initial_good_cards][good_cards]) + " Move: "+str(current_node["action_info"][y]["observed_action"]))
                    else:
                        print("\tPlay " + str(current_node["action_info"][y]["play_count"]) + " cards with " + str(current_node["action_info"][y]["play_good_count"])
                          + " good cards.\tP="+'{:.8f}'.format(current_node["strategy"][y][initial_good_cards][good_cards]) + " Move: "+str(current_node["action_info"][y]["observed_action"]))
                    print(Style.RESET_ALL, end="")


                    

    inp = input("Choose an observed action (#) or go (b)ack > ")[0]
    if inp == 'b' or inp == 'B':
        if len(locator) > 0:
            locator.pop()
    elif inp == 's' or inp == 'S':
        locator.clear()
    else:
        num = int(inp)
        if num >= 0 and num <= current_node["observed_action_count"]:
            locator.append(num)
    
    #Print current node info
    #Prompt for a user navigation
    #Navigate there
