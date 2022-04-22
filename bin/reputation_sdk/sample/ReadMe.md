 # Reputation Sample:
 ## FedAvg_reputation
 - no reputation
 - Output model = FedAvg(received models)

 ## HalfFedAvg_reputation
 - no reputation
 - Output model = 0.5 * previous model + 0.5 * FedAvg(all received models)

 ## sample_reputation
 - Initial reputation: 1
 - Punish the node with least accuracy by reducing 0.05 reputation
 - Output model = 0.5 * previous model + 0.5 * FedAvg(all received models * node reputation)

 ## hcluster_reputation.cpp
 - Initial reputation: 1
 - Punish the node with least accuracy by reducing 0.05 reputation
 - Perform hierarchical clustering on accuracy and reputation (2D space) to select the majority models
 - Output model = 0.5 * previous model + 0.5 * FedAvg(majority models * node reputation)