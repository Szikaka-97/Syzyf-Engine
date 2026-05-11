#include <Messaging.h>

#include <stack>
#include <Scene.h>

void Messenger::Call() {
	(*this->receiver.*this->message)();
}

bool MessageTree::TryFindNode(SceneNode* sceneNode, MessageTree::MessageNode** result) {
	std::stack<SceneNode*> parentChain;

	parentChain.push(sceneNode);

	while (parentChain.top()->GetParent()) {
		parentChain.push(parentChain.top()->GetParent());
	}

	if (parentChain.top() != this->root->content.node) {
		spdlog::warn("TryFindNode: Node is detached from tree - {}", sceneNode->GetID());

		asm("INT3");

		result = nullptr;

		return false;
	}

	*result = this->root;

	parentChain.pop();

	while (!parentChain.empty()) {
		SceneNode* top = parentChain.top();

		bool found = false;

		for (MessageNode* child : (*result)->children) {
			if (child->type == 0 && child->content.node == top) {
				*result = child;

				parentChain.pop();

				found = true;

				break;
			}
		}

		if (!found) {
			spdlog::warn("TryFindNode: Node tree split: last common node - {}", (*result)->content.node->GetID());
	
			result = nullptr;
	
			return false;
		}
	}

	return true;
}

bool MessageTree::TryFindObjectNode(GameObject* obj, MessageNode** result) {
	assert(obj->GetNode());

	return TryFindNode(obj->GetNode(), result);
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

		if (!top->content.node->EnabledSelf()) {
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

void MessageTree::SendMessageInternal(GameObject* obj, int messageId) {
	MessageNode* messagedNode = nullptr;

	if (!TryFindObjectNode(obj, &messagedNode)) {
		spdlog::warn("SendMessageInternal: Node not found - {}", obj->GetID());
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
		if (child->type == 0) {
            RemoveNode(child);
		}

		delete child;
	}

	if (node->parent) {
		std::erase(node->parent->children, node);
	}

	delete node;
}

MessageTree::MessageTree():
root(nullptr) { }

MessageTree::~MessageTree() {
	if (this->root != nullptr) {
		RemoveNode(this->root);
	}
}

void MessageTree::AddNode(SceneNode* node) {
	assert(node != nullptr);

	MessageNode* added = new MessageNode();
	added->content.node = node;
	added->type = 0;
	added->parent = nullptr;

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
	else {
		this->root = added;
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

    if (newParent != nullptr) {
        if (!TryFindNode(newParent, &newParentNode)) {
            spdlog::warn("MoveNode: New node parent not found");
            return;
        }
    }

	if (movedNode->parent) {
		std::erase(movedNode->parent->children, movedNode);
	}

	movedNode->parent = newParentNode;

    if (newParentNode != nullptr) {
	    newParentNode->children.push_back(movedNode);
    }
}

void MessageTree::RemoveMessageReceiver(GameObject* obj) {
	assert(obj != nullptr);

	MessageNode* ownerNode = nullptr;

	if(!TryFindObjectNode(obj, &ownerNode)) {
		spdlog::warn("RemoveMessageReceiver: Node not found - {}", obj->GetNode()->GetID());
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
