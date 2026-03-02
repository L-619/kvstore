
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kvstore.h"










// 1. 红黑树的定义




// 2. 寻找红黑树中的从某个节点开始的的最小节点和最大节点
rbtree_node* rbtree_min(rbtree* T, rbtree_node* node) {
    if (node == T->nil) return T->nil;

    while (node->left != T->nil) {
        node = node->left;
    }

    return node;
}

rbtree_node* rbtree_max(rbtree* T, rbtree_node* node) {
    if (node == T->nil) return T->nil;

    while (node->right != T->nil) {
        node = node->right;
    }

    return node;
}



// 3. 左旋和右旋
void rbtree_left_rotation(rbtree* T, rbtree_node* x) {
    if (x == T->nil) return;
    rbtree_node* y = x->right;

    if (y == T->nil) return;

    // 3.1 
    x->right = y->left;
    if (y->left != T->nil) {
        y->left->parent = x;
    }

    // 3.2 
    y->parent = x->parent;
    if (x->parent == T->nil) {
        T->root = y;
    }
    else if (x == x->parent->left) {
        x->parent->left = y; 
    }
    else if (x == x->parent->right) {
        x->parent->right = y;
    }

    // 3.3 
    y->left = x;
    x->parent = y;
}

void rbtree_right_rotation(rbtree* T, rbtree_node* y) {
    if (y == T->nil) return;
    rbtree_node* x = y->left;

    if (x == T->nil) return;

    // 3.1 
    y->left = x->right;
    if (x->right != T->nil) {
        x->right->parent = y;
    }

    // 3.2 
    x->parent = y->parent;
    if (y->parent == T->nil) {
        T->root = x;
    }
    else if (y == y->parent->left) {
        y->parent->left = x;
    }
    else if (y == y->parent->right) {
        y->parent->right = x;
    }

    // 3.3
    x->right = y;
    y->parent = x;
}


// 4. 红黑树的插入
void rbtree_insert_fixup(rbtree* T, rbtree_node* node) {
    // 只有父节点也是红色节点的时，才需要调整
    while (node->parent->color == RED) {
        // 4.1 父节点是爷爷节点的左子树
        if (node->parent == node->parent->parent->left) {
            rbtree_node* uncle_node = node->parent->parent->right;

            // 4.1.1 叔父节点是红色节点
            if (uncle_node->color == RED) {
                node->parent->parent->color = RED;
                node->parent->color = BLACK;
                uncle_node->color = BLACK;

                node = node->parent->parent;

                // 4.1.2 叔父节点是黑色节点
            }
            else if (uncle_node->color == BLACK) {
                if (node == node->parent->right) {
                    node = node->parent;
                    rbtree_left_rotation(T, node);
                }

                node->parent->color = BLACK;
                node->parent->parent->color = RED;
                rbtree_right_rotation(T, node->parent->parent);
            }

            // 4.2 父节点是爷爷节点的右子树
        }
        else if (node->parent == node->parent->parent->right) {
            rbtree_node* uncle_node = node->parent->parent->left;

            // 4.2.1 叔父节点是红色节点
            if (uncle_node->color == RED) {
                node->parent->parent->color = RED;
                node->parent->color = BLACK;
                uncle_node->color = BLACK;

                node = node->parent->parent;

                // 4.2.2 叔父节点是黑色节点
            }
            else if (uncle_node->color == BLACK) {
                if (node == node->parent->left) {
                    node = node->parent;
                    rbtree_right_rotation(T, node);
                }

                node->parent->color = BLACK;
                node->parent->parent->color = RED;
                rbtree_left_rotation(T, node->parent->parent);
            }
        }
    }

    T->root->color = BLACK;
}

void rbtree_insert(rbtree* T, rbtree_node* node) {
    // 4.1 找到插入位置的父节点
    rbtree_node* x = T->root;
    rbtree_node* y = T->nil;

    while (x != T->nil) {
        y = x;
#if ENABLE_KEY_CHAR
        if (strcmp(node->key, x->key) < 0)
        {
			x = x->left;
        }
        else if (strcmp(node->key, x->key) > 0)
        {
			x=x->right;
        }
        else {
            return;
        }
#else
        if (node->key == x->key) {
            return;    // 一样，不做处理
        }
        else if (node->key < x->key) {
            x = x->left;
        }
        else if (node->key > x->key) {
            x = x->right;
        }
#endif
    }

    // 4.2 插入
    node->parent = y;
    if (y == T->nil) {
        T->root = node;
#if ENABLE_KEY_CHAR
    }
    else if (strcmp(node->key,y->key)<0) {
#else 
    } else if (y->key > node->key) {
#endif
        y->left = node;
    }
    else {
        y->right = node;
    }

    node->left = T->nil;
    node->right = T->nil;
    node->color = RED;    // 默认插入红色

    // 4.3 调整
    rbtree_insert_fixup(T, node);
}


// 5. 红黑树的删除
// 5.1 找到node节点的可代替节点，右子树的最小值or左子树的最大值，默认为右子树的最小值
rbtree_node* rbtree_successor(rbtree* T, rbtree_node* node) {
    if (node->right != T->nil) {
        return rbtree_min(T, node->right);
    }
    else if (node->left != T->nil) {
        return rbtree_max(T, node->left);
    }

    return node;
}

// 5.2 调整
void rbtree_delete_fixup(rbtree* T, rbtree_node* node) {
    while ((node != T->root) && (node->color == BLACK)) {

        // 1. 删除节点为：左子树
        if (node == node->parent->left) {
            rbtree_node* bro_node = node->parent->right;

            // 1.1 兄弟节点为：红色
            if (bro_node->color == RED) {
                bro_node->color = BLACK;
                bro_node->parent->color = RED;

                rbtree_left_rotation(T, node->parent);
                bro_node = node->parent->right;
            }

            // 1.2 兄弟节点为：黑色
            // 1.2.1 兄弟节点的左子树为：黑色，右子树为：黑色
            if ((bro_node->left->color == BLACK) && (bro_node->right->color == BLACK)) {
                bro_node->color = RED;
                node = node->parent;
            }
            else {
                // 1.2.2 兄弟节点的左子树为：红色，右子树为：黑色
                if ((bro_node->left->color == RED) && (bro_node->right->color == BLACK)) {
                    bro_node->left->color = BLACK;
                    bro_node->color = RED;

                    rbtree_right_rotation(T, bro_node);
                    bro_node = node->parent->right;
                }

                // 1.2.3 兄弟节点的左子树: 任意，右子树为：红色
                bro_node->color = node->parent->color;
                node->parent->color = BLACK;
                bro_node->right->color = BLACK;
                rbtree_left_rotation(T, node->parent);

                node = T->root;
            }

            // 2. 删除节点为：右子树
        }
        else if (node == node->parent->right) {
            rbtree_node* bro_node = node->parent->left;

            // 2.1 兄弟节点为：红色
            if (bro_node->color == RED) {
                bro_node->color = BLACK;
                bro_node->parent->color = RED;

                rbtree_right_rotation(T, node->parent);
                bro_node = node->parent->left;
            }

            // 2.2 兄弟节点为：黑色
            // 2.2.1 兄弟节点的左子树为：黑色，右子树为：黑色
            if ((bro_node->left->color == BLACK) && (bro_node->right->color == BLACK)) {
                bro_node->color = RED;
                node = node->parent;
            }
            else {
                // 2.2.2 兄弟节点的左子树为：黑色，右子树为：红色
                if ((bro_node->left->color == BLACK) && (bro_node->right->color == RED)) {
                    bro_node->right->color = BLACK;
                    bro_node->color = RED;

                    rbtree_left_rotation(T, bro_node);
                    bro_node = node->parent->left;
                }

                // 2.2.3 兄弟节点的左子树为：红色，右子树为：任意
                bro_node->color = node->parent->color;
                node->parent->color = BLACK;
                bro_node->left->color = BLACK;
                rbtree_right_rotation(T, node->parent);

                node = T->root;
            }
        }
    }

    node->color = BLACK;
}



// 5.3 删除
rbtree_node* rbtree_delete(rbtree* T, rbtree_node* node) {
    rbtree_node* del_node = T->nil;    // 真正待删除的节点
    rbtree_node* son_node = T->nil;    // 待删除的节点的子树

    // 5.3.1 找到需要删除的节点 del_node
    if ((node->left == T->nil) || (node->right == T->nil)) {
        del_node = node;
    }
    else {
        del_node = rbtree_successor(T, node);
    }

    // 5.3.2 删除 del_node 节点
    if (del_node->left != T->nil) {
        son_node = del_node->left;
    }
    else if (del_node->right != T->nil) {
        son_node = del_node->right;
    }

    //if (son_node != T->nil) son_node->parent = del_node->parent;
    son_node->parent = del_node->parent;

    if (del_node->parent == T->nil) {
        T->root = son_node;
    }
    else if (del_node == del_node->parent->left) {
        del_node->parent->left = son_node;
    }
    else if (del_node == del_node->parent->right) {
        del_node->parent->right = son_node;
    }

    // 5.3.3 用 del_node 节点，覆盖真正待删除的node节点
    if (del_node != node) {
#if ENABLE_KEY_CHAR
		void *tmp =node->key;
		node->key = del_node->key;
		del_node->key = tmp;

        tmp = node->value;
		node->value = del_node->value;
		del_node->value = tmp;
#else
        node->key = del_node->key;
        node->value = del_node->value;
#endif
    }

    // 5.3.4 如果正真删除的y节点为红色，则不需要进行调整；如果为黑色，则需要进行调整，以满足红黑树的性质
    if (del_node->color == BLACK) {
        rbtree_delete_fixup(T, son_node);
    }
    // 5.3.5 返回真正删除的节点
    return del_node;
}



// 6. 红黑树的查找
rbtree_node* rbtree_search(rbtree* T, KEY_TYPE key) {
    rbtree_node* node = T->root;
    while (node != T->nil) {
#if ENABLE_KEY_CHAR
        if (strcmp(key, node->key) == 0) return node;
        else if (strcmp(node->key, key) < 0)
        {
            node = node->right;
        }
        else { node = node->left; }
#else
        if (key == node->key) return node;
        else if (key > node->key) node = node->right;
        else if (key < node->key) node = node->left;
    }
#endif
    }
    return T->nil;
}


// 7. 红黑树的遍历
void rbtree_traversal(rbtree* T, rbtree_node* node) {
    if (node != T->nil) {
        rbtree_traversal(T, node->left);
		printf("key: %s, color: %d\n", node->key, node->color);
        rbtree_traversal(T, node->right);
    }
}




// 红黑树的6个接口api
int kvstore_rbtree_create(rbtree *tree)//红黑树的初始化
{

	if (!tree) return -1;

    memset(tree, 0, sizeof(rbtree));


	tree->nil = (rbtree_node*)kvstore_malloc(sizeof(rbtree_node));
    tree->nil->key = malloc(1);

    *(tree->nil->key) = '\0';

	tree->nil->color = BLACK;
 
	tree->root = tree->nil;
	printf("rbtree create success\n");
    return 0;
}
void kvstore_rbtree_destroy(rbtree *tree)//红黑树的销毁
{ 
    if (!tree)return ;
    if (tree->nil->key) kvstore_free(tree->nil->key);
	rbtree_node* node = tree->root;
    while(node != tree->nil) {
        node = rbtree_min(tree, tree->root);
        if (node == tree->nil)
        {
            break;
        }
		node = rbtree_delete(tree, node);
        if (!node)
        {
            kvstore_free(node->key);
            kvstore_free(node->value);
            kvstore_free(node);
        }

	}

}
int rbtree_kv_set(rbtree*tree,char* key, char* value)//红黑树set操作
{
	rbtree_node* node = (rbtree_node*)kvstore_malloc(sizeof(rbtree_node));
    if (!node)return -1;
    node->key = kvstore_malloc(strlen(key) + 1);
    if (node->key == NULL)
    {
        kvstore_free(node);
		return -1;
    }
	memset(node->key, 0, strlen(key) + 1);
    strcpy(node->key, key);
    
    node->value = kvstore_malloc(strlen(value) + 1);
    if (node->value == NULL)
    {
        kvstore_free(node->key);
        kvstore_free(node);
        return -1;
	}
	memset(node->value, 0, strlen(value) + 1);
    strcpy((char *)node->value, value);
	rbtree_insert(tree, node);

    tree->count++;
    return 0;
}
char * rbtree_kv_get(rbtree *tree,char* key)//红黑树的get操作
{
    rbtree_node* node = rbtree_search(tree, key);
    if (node == tree->nil) return NULL;
	return  node->value;
}
int  rbtree_kv_delete(rbtree* tree,char* key)//红黑树的delete操作
{
	rbtree_node* node = rbtree_search(tree, key);
	if (node == tree->nil) return -1;
	rbtree_node* del_node = rbtree_delete(tree, node);
    if(!del_node)
    {
        kvstore_free(del_node->key);
        kvstore_free(del_node->value);
        kvstore_free(del_node);
	}


    tree->count--;
    return 0;
}
int rbtree_kv_modify(rbtree* tree, char* key, char* value)//红黑树的mod
{
    rbtree_node* node = rbtree_search(tree, key);
    if (node == tree->nil) return -1;
    char* tmp = node->value;
    kvstore_free(tmp);

	node->value = kvstore_malloc(strlen(value) + 1);
    if(node->value == NULL)
    {
        return -1;
	}
    strcpy(node->value, value);
    return 0;
}
int rbtree_kv_count(rbtree* tree)
{
	return tree->count;
}
rbtree Tree;