#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SIZE 1024

struct Node {
  char symbol;
  int frequency;
  struct Node *right;
  struct Node *left;
};

struct Heap {
  struct Node **buf;
  size_t size;
};

void swap(struct Node *a, struct Node *b) {
  struct Node temp = *a;
  *a               = *b;
  *b               = temp;
}

void sort_heap(struct Heap *heap) {
  for (int i = 0; i < heap->size; i++) {
    int node = i;
    while (node > 0) {
      int parent = node % 2 == 0 ? (node - 2) / 2 : (node - 1) / 2;
      if (heap->buf[parent]->frequency > heap->buf[node]->frequency) {
        swap(heap->buf[parent], heap->buf[node]);
        node = parent;
      } else {
        break;
      }
    }
  }
}

void push_heap(struct Heap *heap, struct Node *node) {
  if (heap->size == MAX_SIZE) return;
  heap->buf[heap->size] = node;
  heap->size++;
  sort_heap(heap);
}

void pop_heap(struct Heap *heap) {
  if (!heap->size) return;
  swap(heap->buf[0], heap->buf[heap->size - 1]);
  heap->buf[heap->size - 1]->symbol    = 0;
  heap->buf[heap->size - 1]->frequency = 0;
  heap->size--;
}

struct Node *top_heap(struct Heap *heap) {
  if (heap->size == 0) return NULL;
  return heap->buf[0];
}

char *configure_result(struct Node *node, char *result) {}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Invalid input data\n");
    return 1;
  }

  char *data        = argv[1];
  struct Node **buf = (struct Node **)calloc(MAX_SIZE, sizeof(struct Node));
  struct Heap heap  = {buf, 0};

  int frequencies[128];
  for (int i = 0; i < 128; i++) {
    frequencies[i] = 0;
  }
  for (int i = 0; i < strlen(data); i++) {
    frequencies[data[i]]++;
  }
  for (int i = 0; i < 128; i++) {
    struct Node *node = (struct Node *)malloc(sizeof(struct Node));
    node->symbol      = i;
    node->frequency   = frequencies[i];
    node->right       = NULL;
    node->left        = NULL;
    if (frequencies[i] != 0) push_heap(&heap, node);
  }

  while (heap.size > 1) {
    struct Node *a = top_heap(&heap);
    pop_heap(&heap);
    struct Node *b = top_heap(&heap);
    pop_heap(&heap);
    struct Node *parent = (struct Node *)malloc(sizeof(struct Node));
    parent->symbol      = 0;
    parent->frequency   = a->frequency + b->frequency;
    parent->right       = a;
    parent->left        = b;
    push_heap(&heap, parent);
  }

  char *result = (char *)calloc(MAX_SIZE, sizeof(char));
  result[0]    = '\0';
  free(buf);
  return 0;
}
