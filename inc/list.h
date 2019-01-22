struct list;
struct list_element;

struct list {
    struct list_node *front; //pointer to the first node in the list
    struct list_node *back; //pointer to the last node in the list
    size_t size; //how many nodes there are in the list
};

struct list_node {
    struct list_node *prev; //previous node in the list (NULL if this is the first one)
    struct list_node *next; //next node in the list (NULL if this is the last one)
    void *data; //the payload of this node
    size_t size; //the total size of the reserved memory for the payload
};
