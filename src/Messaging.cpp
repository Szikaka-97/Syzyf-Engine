#include "Profiler.h"
#include "TypeInfo.h"
#include <Messaging.h>

#include <stack>
#include <Scene.h>

void Messenger::Call() {
	(*this->receiver.*this->message)();
}

bool MessageTree::TryFindNode(SceneNode* sceneNode, MessageTree::MessengerNode** result) {
	std::stack<SceneNode*> parentChain;

	parentChain.push(sceneNode);

	while (parentChain.top()->GetParent()) {
		parentChain.push(parentChain.top()->GetParent());
	}

	if (parentChain.top() != this->root->content.node) {
		spdlog::warn("TryFindNode: Node is detached from tree - {}", sceneNode->GetID());

		result = nullptr;

		return false;
	}

	*result = this->root;

	parentChain.pop();

	while (!parentChain.empty()) {
		SceneNode* top = parentChain.top();

		bool found = false;

		for (MessengerNode* child : (*result)->children) {
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

bool MessageTree::TryFindObjectNode(GameObject* obj, MessengerNode** result) {
	assert(obj->GetNode());

	return TryFindNode(obj->GetNode(), result);
}

void MessageTree::MessageNodeInternal(SceneNode* node, int messageId) {
	MessengerNode* messageRoot = nullptr;

	if (!TryFindNode(node, &messageRoot)) {
		spdlog::warn("MessageNodeInternal: Node not found - {}", node->GetID());
		return;
	}

	for (MessengerNode* child : messageRoot->children) {
		if (child->type == messageId && child->content.msg.receiver->EnabledSelf()) {
			child->content.msg.Call();
		}
	}
}

void MessageTree::PropagateMessageInternal(SceneNode* startNode, int messageId) {
	assert(startNode != nullptr);

	MessengerNode* messageRoot = nullptr;

	if (!TryFindNode(startNode, &messageRoot)) {
		spdlog::warn("PropagateMessageInternal: Node not found - {}", startNode->GetID());
		return;
	}

	std::stack<MessengerNode*> nodeStack;
	std::stack<Messenger> messengers;

	nodeStack.push(messageRoot);

	while (!nodeStack.empty()) {
		MessengerNode* top = nodeStack.top();
		nodeStack.pop();

		if (!top->content.node->EnabledSelf()) {
			continue;
		}

		for (MessengerNode* child : top->children) {
			if (child->type == 0) {
				nodeStack.push(child);
			}
			else if (child->type == messageId && child->content.msg.receiver->IsEnabled()){
				messengers.push(child->content.msg);
			}
		}
	}

	while (!messengers.empty()) {
		Profiler::Push(TypeInfo::GetTypeInfo(typeid(*(messengers.top().receiver))).name);

		messengers.top().Call();
		
		Profiler::Pop();

		messengers.pop();
	}
}

void MessageTree::SendMessageInternal(GameObject* obj, int messageId) {
	MessengerNode* messagedNode = nullptr;

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

void MessageTree::AddMessageReceiverInternal(MessengerNode* node, Messenger msg, int type) {
	MessengerNode* added = new MessengerNode();
	added->content.msg = msg;
	added->type = type;
	added->parent = node;
	
	node->children.push_back(added);
}

void MessageTree::RemoveNode(MessengerNode* node) {
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

	MessengerNode* added = new MessengerNode();
	added->content.node = node;
	added->type = 0;
	added->parent = nullptr;

	if (node->GetParent()) {
		MessengerNode* parent = nullptr;

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

	MessengerNode* removed = nullptr;

	if (!TryFindNode(node, &removed)) {
		spdlog::warn("RemoveNode: Node not found - {}", node->GetID());
		return;
	}

	RemoveNode(removed);
}

void MessageTree::MoveNode(SceneNode* node, SceneNode* newParent) {
	assert(node != nullptr);
	assert(newParent != nullptr);

	MessengerNode* movedNode = nullptr;
	MessengerNode* newParentNode = nullptr;

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

	MessengerNode* ownerNode = nullptr;

	if(!TryFindObjectNode(obj, &ownerNode)) {
		spdlog::warn("RemoveMessageReceiver: Node not found - {}", obj->GetNode()->GetID());
		return;
	}

	std::vector<MessengerNode*> newChildren;

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
