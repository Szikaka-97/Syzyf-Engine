#pragma once

#include <vector>
#include <concepts>
#include <assert.h>

#define DEFINE_MESSAGE(MessageName) \
template<class T> \
concept MessageName##Receiver = requires (T a) { \
	{ a.MessageName() } -> std::same_as<void>; \
} && std::derived_from<T, GameObject>;\
\
namespace Message { \
	struct MessageName : public MessageTag { constexpr static int id = LOCAL_COUNTER; }; \
} 

#define DEFINE_MESSAGE_CREATOR(MessageName) \
template<class T> \
	requires std::derived_from<T, GameObject> \
inline void Add##MessageName(MessageNode* node, T* object) { } \
template<class T> \
	requires std::derived_from<T, GameObject> && MessageName##Receiver<T> \
inline void Add##MessageName(MessageNode* node, T* object) { \
	AddMessageReceiverInternal(node, { object, reinterpret_cast<MessageHandle>(&T::MessageName) }, Message::MessageName::id); \
} \

class GameObject;

class MessageTag { };

enum { COUNTER_BASE = __COUNTER__ };

#define LOCAL_COUNTER (__COUNTER__ - COUNTER_BASE)

typedef void (GameObject::*MessageHandle)();

class SceneNode;
class Scene;

DEFINE_MESSAGE(Update);
DEFINE_MESSAGE(FixedUpdate);
DEFINE_MESSAGE(Render);
DEFINE_MESSAGE(DrawGizmos);
DEFINE_MESSAGE(OnEnable);
DEFINE_MESSAGE(OnDisable);
DEFINE_MESSAGE(Awake);

struct Messenger {
	GameObject* receiver;
	MessageHandle message;

	void Call();
};

class MessageTree {
public:
	struct MessageNode {
		MessageNode* parent;
		int type;
		std::vector<MessageNode*> children;

		union {
			Messenger msg;
			SceneNode* node;
		} content;

		MessageNode() = default;
		~MessageNode() = default;
	};

	MessageNode* root;

	bool TryFindNode(SceneNode* sceneNode, MessageNode** result);
	bool TryFindObjectNode(GameObject* obj, MessageNode** result);

	void PropagateMessageInternal(SceneNode* startNode, int messageId);
	
	void SendMessageInternal(GameObject* obj, int messageId);

	void RemoveNode(MessageNode* node);

	void AddMessageReceiverInternal(MessageNode* node, Messenger msg, int type);

	DEFINE_MESSAGE_CREATOR(Update);
	DEFINE_MESSAGE_CREATOR(FixedUpdate);
	DEFINE_MESSAGE_CREATOR(Render);
	DEFINE_MESSAGE_CREATOR(DrawGizmos);
	DEFINE_MESSAGE_CREATOR(OnEnable);
	DEFINE_MESSAGE_CREATOR(OnDisable);
	DEFINE_MESSAGE_CREATOR(Awake);
public:
	MessageTree();
	~MessageTree();
	
	template<typename T>
		requires std::derived_from<T, MessageTag>
	void PropagateMessage(SceneNode* startNode);

	template<typename T>
		requires std::derived_from<T, MessageTag>
	void MessageObject(GameObject* obj);

	void AddNode(SceneNode* node);

	void RemoveNode(SceneNode* node);

	void MoveNode(SceneNode* node, SceneNode* newParent);

	template<typename T>
		requires std::derived_from<T, GameObject>
	void AddMessageReceiver(T* obj);

	void RemoveMessageReceiver(GameObject* obj);
};

template<typename TMessage>
		requires std::derived_from<TMessage, MessageTag>
void MessageTree::PropagateMessage(SceneNode* startNode) {
	PropagateMessageInternal(startNode, TMessage::id);
}

template<typename TMessage>
		requires std::derived_from<TMessage, MessageTag>
void MessageTree::MessageObject(GameObject* obj) {
	SendMessageInternal(obj, TMessage::id);
}

template<typename T>
	requires std::derived_from<T, GameObject>
void MessageTree::AddMessageReceiver(T* obj) {
	assert(obj != nullptr);

	MessageNode* ownerNode = nullptr;

	if (!TryFindObjectNode(obj, &ownerNode)) {
		return;
	}

	AddUpdate(ownerNode, obj);
	AddFixedUpdate(ownerNode, obj);
	AddRender(ownerNode, obj);
	AddDrawGizmos(ownerNode, obj);
	AddOnEnable(ownerNode, obj);
	AddOnDisable(ownerNode, obj);
	AddAwake(ownerNode, obj);
}
