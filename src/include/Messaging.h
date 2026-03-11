#pragma once

#include <vector>
#include <concepts>
#include <unordered_map>
#include <assert.h>

#define DEFINE_MESSAGE(MessageName) \
template<class T> \
concept MessageName##Receiver = requires (T a) { \
	{ a.MessageName() } -> std::same_as<void>; \
} && std::derived_from<T, MessageReceiver>;\
\
namespace Message { \
	struct MessageName : public MessageTag { constexpr static int id = LOCAL_COUNTER; }; \
} 

#define DEFINE_MESSAGE_CREATOR(MessageName) \
template<class T> \
	requires std::derived_from<T, MessageReceiver> \
inline void Add##MessageName(MessageNode* node, T* object) { } \
template<class T> \
	requires std::derived_from<T, MessageReceiver> && MessageName##Receiver<T> \
inline void Add##MessageName(MessageNode* node, T* object) { \
	AddMessageReceiverInternal(node, { object, reinterpret_cast<MessageHandle>(&T::MessageName) }, Message::MessageName::id); \
} \

class MessageReceiver {
public: virtual ~MessageReceiver() = default;
};
class MessageTag { };

enum { COUNTER_BASE = __COUNTER__ };

#define LOCAL_COUNTER (__COUNTER__ - COUNTER_BASE)

typedef void (MessageReceiver::*MessageHandle)();

class SceneNode;
class Scene;

DEFINE_MESSAGE(Update);
DEFINE_MESSAGE(Render);
DEFINE_MESSAGE(DrawGizmos);
DEFINE_MESSAGE(OnEnable);
DEFINE_MESSAGE(OnDisable);
DEFINE_MESSAGE(Awake);

struct Messenger {
	MessageReceiver* receiver;
	MessageHandle message;

	void Call();
};

class MessageTree {
private:
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

	std::unordered_map<int, MessageNode*> quickLookup;

	bool TryFindNode(SceneNode* sceneNode, MessageNode** result);

	void PropagateMessageInternal(SceneNode* startNode, int messageId);
	
	void SendMessageInternal(MessageReceiver* obj, SceneNode* owner, int messageId);

	void RemoveNode(MessageNode* node);

	void AddMessageReceiverInternal(MessageNode* node, Messenger msg, int type);

	DEFINE_MESSAGE_CREATOR(Update);
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
	void MessageObject(MessageReceiver* obj, SceneNode* owner);

	void AddNode(SceneNode* node);

	void RemoveNode(SceneNode* node);

	void MoveNode(SceneNode* node, SceneNode* newParent);

	template<typename T>
		requires std::derived_from<T, MessageReceiver>
	void AddMessageReceiver(T* obj, SceneNode* owner);

	void RemoveMessageReceiver(MessageReceiver* obj, SceneNode* owner);

	void SwapNode(SceneNode* current, SceneNode* changed);
};

template<typename TMessage>
		requires std::derived_from<TMessage, MessageTag>
void MessageTree::PropagateMessage(SceneNode* startNode) {
	PropagateMessageInternal(startNode, TMessage::id);
}

template<typename TMessage>
		requires std::derived_from<TMessage, MessageTag>
void MessageTree::MessageObject(MessageReceiver* obj, SceneNode* owner) {
	SendMessageInternal(obj, owner, TMessage::id);
}

template<typename T>
	requires std::derived_from<T, MessageReceiver>
void MessageTree::AddMessageReceiver(T* obj, SceneNode* owner) {
	assert(obj != nullptr);
	assert(owner != nullptr);

	MessageNode* ownerNode = nullptr;

	if (!TryFindNode(owner, &ownerNode)) {
		return;
	}

	AddUpdate(ownerNode, obj);
	AddRender(ownerNode, obj);
	AddDrawGizmos(ownerNode, obj);
	AddOnEnable(ownerNode, obj);
	AddOnDisable(ownerNode, obj);
  AddAwake(ownerNode, obj);
}
