#include <Messaging.h>

#include <stack>
#include <Scene.h>

void Messenger::Call() {
	(*this->receiver.*this->message)();
}

bool MessageTree::TryFindNode(SceneNode* sceneNode, MessageTree::MessageNode** result) {
	auto it = this->quickLookup.find(sceneNode->GetID());

	if (it == this->quickLookup.end()) {
		return false;
	}
	else {
		*result = it->second;

		return true;
	}
}

void MessageTree::PropagateMessageInternal(SceneNode* startNode, int messageId) {
	assert(startNode != nullptr);

	MessageNode* messageRoot = nullptr;

	if (!TryFindNode(startNode, &messageRoot)) {
		spdlog::warn("PropagateMessageInternal: Node not found - {}", startNode->GetID());
		return;
	}

	std::stack<MessageNode*> nodeStack;
	std::stack<Messenger> messengers;

	nodeStack.push(messageRoot);

	while (!nodeStack.empty()) {
		MessageNode* top = nodeStack.top();
		nodeStack.pop();

		if (!top->content.node->IsEnabled()) {
			continue;
		}

		for (MessageNode* child : top->children) {
			if (child->type == 0) {
				nodeStack.push(child);
			}
			else if (child->type == messageId){
				messengers.push(child->content.msg);
			}
		}
	}

	while (!messengers.empty()) {
		messengers.top().Call();
		messengers.pop();
	}
}

void MessageTree::SendMessageInternal(MessageReceiver* obj, SceneNode* owner, int messageId) {
	MessageNode* messagedNode = nullptr;

	if (!TryFindNode(owner, &messagedNode)) {
		spdlog::warn("SendMessageInternal: Node not found - {}", owner->GetID());
		return;
	}

	for (auto& child : messagedNode->children) {
		if (child->type == messageId && child->content.msg.receiver == obj) {
			child->content.msg.Call();

			break;
		}
	}
}

void MessageTree::AddMessageReceiverInternal(MessageNode* node, Messenger msg, int type) {
	MessageNode* added = new MessageNode();
	added->content.msg = msg;
	added->type = type;
	added->parent = node;
	
	node->children.push_back(added);
}

void MessageTree::RemoveNode(MessageNode* node) {
	for (auto child : node->children) {
		// if (child->type == 0) {
		// 	RemoveNode(child);
		// }

		delete child;
	}
}

MessageTree::MessageTree():
root(nullptr) { }

MessageTree::~MessageTree() {
	if (root != nullptr) {
		RemoveNode(root);
	}
}

void MessageTree::AddNode(SceneNode* node) {
	assert(node != nullptr);

	MessageNode* added = new MessageNode();
	added->content.node = node;
	added->type = 0;
	added->parent = nullptr;
	
	this->quickLookup[node->GetID()] = added;

	if (node->GetParent()) {
		MessageNode* parent = nullptr;

		if (TryFindNode(node->GetParent(), &parent)) {
			added->parent = parent;
		
			parent->children.push_back(added);
		}
		else {
			spdlog::warn("AddNode: Node not found - {}", node->GetID());
		}
	}
}

void MessageTree::RemoveNode(SceneNode* node) {
	assert(node != nullptr);

	MessageNode* removed = nullptr;

	if (!TryFindNode(node, &removed)) {
		spdlog::warn("RemoveNode: Node not found - {}", node->GetID());
		return;
	}

	RemoveNode(removed);

	if (removed->parent) {
		std::erase(removed->parent->children, removed);
	}
}

void MessageTree::MoveNode(SceneNode* node, SceneNode* newParent) {
	assert(node != nullptr);
	assert(newParent != nullptr);

	MessageNode* movedNode = nullptr;
	MessageNode* newParentNode = nullptr;

	if (!TryFindNode(node, &movedNode)) {
		spdlog::warn("MoveNode: Node not found - {}", node->GetID());
		return;
	}

	if (!TryFindNode(newParent, &newParentNode)) {
		spdlog::warn("MoveNode: New node parent not found");
		return;
	}

	if (movedNode->parent) {
		std::erase(movedNode->parent->children, movedNode);
	}

	movedNode->parent = newParentNode;

	newParentNode->children.push_back(movedNode);
}

void MessageTree::RemoveMessageReceiver(MessageReceiver* obj, SceneNode* owner) {
	assert(obj != nullptr);
	assert(owner != nullptr);

	MessageNode* ownerNode = nullptr;

	if(!TryFindNode(owner, &ownerNode)) {
		spdlog::warn("RemoveMessageReceiver: Node not found - {}", owner->GetID());
		return;
	}

	std::vector<MessageNode*> newChildren;

	for (auto child : ownerNode->children) {
		if (child->type != 0 && child->content.msg.receiver == obj) {
			delete child;
		}
		else {
			newChildren.push_back(child);
		}
	}

	ownerNode->children = newChildren;
}

void MessageTree::SwapNode(SceneNode* current, SceneNode* changed) {
	assert(current);
	assert(changed);

	MessageNode* currentNode = nullptr;
	if (!TryFindNode(current, &currentNode)) {
		spdlog::warn("SwapNode: Node not found - {}", current->GetID());
		return;
	}

	MessageNode* added = new MessageNode();
	added->content.node = changed;
	added->type = 0;
	added->parent = currentNode->parent;

	for (auto child : currentNode->children) {
		child->parent = added;
		added->children.push_back(child);
	}

	this->quickLookup.erase(currentNode->content.node->GetID());
	this->quickLookup[changed->GetID()] = added;

	delete currentNode;
}