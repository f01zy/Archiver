#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SIZE 1024
#define SYMBOLS  128

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

struct Code {
  int code;
  size_t size;
};

void swap(struct Node **a, struct Node **b) {
  struct Node *temp = *a;
  *a                = *b;
  *b                = temp;
}

void sort_heap(struct Heap *heap) {
  for (int i = 0; i < heap->size; i++) {
    int node = i;
    while (node > 0) {
      int parent = node % 2 == 0 ? (node - 2) / 2 : (node - 1) / 2;
      if (heap->buf[parent]->frequency > heap->buf[node]->frequency) {
        swap(&heap->buf[parent], &heap->buf[node]);
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
  swap(&heap->buf[0], &heap->buf[heap->size - 1]);
  heap->size--;
}

struct Node *top_heap(struct Heap *heap) {
  if (!heap->size) return NULL;
  return heap->buf[0];
}

void configure_codes(struct Node *node, struct Code codes[SYMBOLS], int curr, int size) {
  if (!node->left && !node->right) {
    codes[node->symbol] = (struct Code){curr, size};
    return;
  }
  if (node->right) configure_codes(node->right, codes, (curr << 1) | 1, size + 1);
  if (node->left) configure_codes(node->left, codes, curr << 1, size + 1);
}

void write_code_to_result(unsigned char result[MAX_SIZE], struct Code code, int *i, int *j) {
  int k = 0;
  while (k < code.size) {
    if (*i > 7) {
      *i = 0;
      (*j)++;
    }
    result[*j] |= ((code.code >> (code.size - k++ - 1)) & 1) << (*i)++;
  }
}

void archive(char *data) {
  struct Node **buf = (struct Node **)calloc(MAX_SIZE, sizeof(struct Node));
  struct Heap heap  = {buf, 0};

  int frequencies[SYMBOLS] = {0};
  for (int i = 0; i < strlen(data); i++) {
    frequencies[data[i]]++;
  }
  for (int i = 0; i < SYMBOLS; i++) {
    struct Node *node = (struct Node *)malloc(sizeof(struct Node));
    node->symbol      = i;
    node->frequency   = frequencies[i];
    node->right       = NULL;
    node->left        = NULL;
    if (frequencies[i]) push_heap(&heap, node);
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

  struct Node *head          = top_heap(&heap);
  struct Code codes[SYMBOLS] = {0};
  configure_codes(head, codes, 0, 0);

  unsigned char result[MAX_SIZE] = {0};
  int i = 0, j = 0;
  for (int k = 0; k < strlen(data); k++) {
    struct Code code = codes[data[k]];
    if (!code.size) continue;
    write_code_to_result(result, code, &i, &j);
  }

  printf("Compressed data (HEX): ");
  for (int k = 0; k <= j; k++) {
    printf("%02X ", (unsigned char)result[k]);
  }
  printf("\n");

  free(buf);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Invalid input data\n");
    return 1;
  }
  char *data = argv[1];
  archive(data);
  return 0;
}
