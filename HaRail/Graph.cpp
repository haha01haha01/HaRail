#include "Graph.h"

Graph::Graph(const IDataSource *ids)
: nodes(),
nodesByStation(),
edges(),
start_node(nullptr),
end_node(nullptr)
{
	for (Train *train : ids->getTrains()) {
		Node *source = getNodeOrAdd(train->getSource(), train->getSourceTime());
		Node *dest = getNodeOrAdd(train->getDest(), train->getDestTime());
		Edge *edge = new Edge(train, source, dest, train->getCost());
		edges.push_back(edge);
		source->getEdges().push_back(edge);
	}

	for (pair<Station *, unordered_map<int, Node *>> p : nodesByStation) {
		vector<Node *> node_arr(p.second.size());
		for (pair<int, Node *> p2 : p.second) {
			node_arr.push_back(p2.second);
		}
		sort(node_arr.begin(), node_arr.end(), [](const Node *& first, const Node *& second) -> bool { return first->getStationTime() > second->getStationTime(); });
		for (unsigned int i = 0; i < node_arr.size() - 1; i++) {
			Node *source = node_arr[i];
			Node *dest = node_arr[i + 1];
			Edge *edge = new Edge(nullptr, source, dest, dest->getStationTime() - source->getStationTime());
			edges.push_back(edge);
			source->getEdges().push_back(edge);
		}
	}
}

Graph::~Graph()
{
	for (Node *node : nodes) {
		delete node;
	}
	for (Edge *edge : edges) {
		delete edge;
	}
}

void Graph::dijkstra(Station *source_station, int start_time, Station *dest_station)
{
	auto pr = [](const pair<Node *, int>& first, const pair<Node *, int>& second) { return first.second > second.second; };
	priority_queue<pair<Node *, int>, vector<pair<Node *, int>>, decltype(pr)> pq(pr);

	start_node = nodesByStation[source_station][start_time];
	start_node->setBestCost(0);
	start_node->setVisited(true);
	Node *curr = start_node;
	while (curr->getStation() != dest_station) {
		int curr_tid = getCurrentTrain(curr);
		for (Edge *edge : curr->getEdges()) {
			if (!edge->getDest()->getVisited()) {
				int cost = curr->getBestCost() + edge->getCost();

				if (curr_tid != -1 && edge->getTrain() && edge->getTrain()->getTrainId() != curr_tid) {
					cost += 1;
				}

				if (edge->getDest()->getBestCost() > cost) {
					edge->getDest()->setBestCost(cost);
					edge->getDest()->setBestSource(edge);
					pq.push(pair<Node *, int>(edge->getDest(), cost));
				}
			}
		}
		curr->setVisited(true);

		int pair_cost;
		Node *pair_node;
		do {
			pair<Node *, int>& p = pq.top();
			pq.pop();
			pair_node = p.first;
			pair_cost = p.second;
		} while (pair_node->getBestCost() != pair_cost);
		curr = pair_node;
	}
	end_node = curr;
}

vector<Train *> Graph::backtraceRoute()
{
	Node *curr = end_node;
	while (curr != start_node) {
		Edge *best_edge = curr->getBestSource();
		best_edge->getSource()->setBestDest(best_edge);
		curr = best_edge->getSource();
	}

	vector<Train *> result;
	while (curr != end_node) {
		Edge *best_edge = curr->getBestDest();
		if (best_edge->getTrain()) {
			result.push_back(best_edge->getTrain());
		}
		curr = best_edge->getDest();
	}
	return result;
}

void Graph::resetGraph()
{
	for (Node *node : nodes) {
		node->setBestSource(nullptr);
		node->setBestDest(nullptr);
		node->setBestCost(Node::UNEXPLORED_COST);
		node->setVisited(false);
	}
}

int Graph::getCurrentTrain(Node *node) const
{
	Node *curr = node;
	Edge *edge = curr->getBestSource();
	while (edge && !edge->getTrain()) {
		curr = edge->getSource();
		edge = curr->getBestSource();
	}
	return edge ? edge->getTrain()->getTrainId() : -1;
}

Node *Graph::getNodeOrAdd(Station *station, int time)
{
	Node *& node_ref = nodesByStation[station][time];
	if (node_ref == nullptr) {
		Node *node = new Node(station, time);
		node_ref = node;
		nodes.push_back(node);
	}
	return node_ref;
}