#inlcude "list.h" //header for this inplementation file

//get element number _n in the list
//struct list_node *list_get_at(struct list *ls, size_t n) {
//    //TODO
//}

struct list_node *list_get_next(struct list *ls, struct list_node *node) {
    //TODO test
    if(node != ls->back) {return node->next;}
    else {return NULL;}
}

struct list_node *list_get_prev(struct list *ls, struct list_node *node) {
    //TODO test
    if(node != ls->front) {return node->prev;}
    else {return NULL;}
}

//inserts the new node right after given head node in the list
void list_insert(struct list *ls, struct list_node *head, struct list_node *new)
{
    //TODO test
    new->prev = head;
    new->next = head->next;
    head->next = new;
    ++ls->size;
}

void list_push_back(struct list *ls, struct list_node *node)
{
    //TODO test
    ls->back->next = node;
    node->prev = ls->back;
    ls->back = node;
    ++ls->size;
}

//TODO list_push_front(struct list *ls, struct list_node *node)
//TODO list_pop_back(struct list *ls)
//TODO list_pop_front(struct list *ls)

void list_delete_node(struct list *ls, struct list_node *node)
{
    //TODO test
    //if not at the beginning of list - close the would-be gap
    if(node->prev != NULL) {node->prev->next = node->next;}
    free_list_node(node);
    --ls->size;
}

struct list_node *create_list_node(void *data, size_t data_sz)
{
    //TODO test
    struct list_node *node = NULL;
    node = (struct list_node)malloc(sizeof (struct list_node));
    node->data = data;
    node->size = data_sz;
}

void *free_list_node(struct list_node *node)
{
    //TODO test
    free(node->data);
    free(node);
}
