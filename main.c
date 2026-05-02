#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HEAP_SIZE     1024
#define MAX_DATA_SIZE     65536
#define MAX_FILENAME_SIZE 128
#define SYMBOLS           256

struct Node {
  unsigned char symbol;
  int frequency;
  struct Node *right;
  struct Node *left;
};

struct Heap {
  struct Node **buf;
  int size;
};

struct Code {
  int code;
  int size;
};

void swap(struct Node **a, struct Node **b) {
  struct Node *temp = *a;
  *a                = *b;
  *b                = temp;
}

void shift_up(struct Heap *heap) {
  int i = heap->size - 1;
  while (i > 0) {
    int parent = i % 2 == 0 ? (i - 2) / 2 : (i - 1) / 2;
    if (heap->buf[parent]->frequency > heap->buf[i]->frequency) {
      swap(&heap->buf[parent], &heap->buf[i]);
      i = parent;
    } else {
      break;
    }
  }
}

void shift_down(struct Heap *heap) {
  int i = 0;
  while (i < heap->size - 1) {
    int left     = i * 2 + 1;
    int right    = i * 2 + 2;
    int smallest = i;
    if (left < heap->size && heap->buf[left]->frequency < heap->buf[smallest]->frequency) smallest = left;
    if (right < heap->size && heap->buf[right]->frequency < heap->buf[smallest]->frequency) smallest = right;
    if (smallest == i) break;
    swap(&heap->buf[i], &heap->buf[smallest]);
    i = smallest;
  }
}

void push_heap(struct Heap *heap, struct Node *node) {
  if (heap->size == MAX_HEAP_SIZE) return;
  heap->buf[heap->size] = node;
  heap->size++;
  shift_up(heap);
}

void pop_heap(struct Heap *heap) {
  if (!heap->size) return;
  swap(&heap->buf[0], &heap->buf[heap->size - 1]);
  heap->size--;
  shift_down(heap);
}

void free_heap(struct Heap *heap) {
  for (int i = 0; i < MAX_HEAP_SIZE; i++) {
    if (heap->buf[i]) free(heap->buf[i]);
  }
  free(heap->buf);
}

struct Node *top_heap(struct Heap *heap) {
  if (!heap->size) return NULL;
  return heap->buf[0];
}

void configure_codes(struct Node *node, struct Code codes[SYMBOLS], int curr, int size) {
  if (!node) return;
  if (!node->left && !node->right) {
    codes[node->symbol] = (struct Code){curr, size};
    return;
  }
  if (node->right) configure_codes(node->right, codes, (curr << 1) | 1, size + 1);
  if (node->left) configure_codes(node->left, codes, curr << 1, size + 1);
}

void write_code_to_result(unsigned char output[MAX_DATA_SIZE], struct Code code, int *i, int *j) {
  int k = 0;
  while (k < code.size) {
    if (*i > 7) {
      *i = 0;
      (*j)++;
    }
    output[*j] |= ((code.code >> (code.size - k++ - 1)) & 1) << (*i)++;
  }
}

struct Node *configure_huffman_tree(struct Heap *heap) {
  while (heap->size > 1) {
    struct Node *a = top_heap(heap);
    pop_heap(heap);
    struct Node *b = top_heap(heap);
    pop_heap(heap);
    struct Node *parent = (struct Node *)malloc(sizeof(struct Node));
    parent->symbol      = 0;
    parent->frequency   = a->frequency + b->frequency;
    parent->right       = a;
    parent->left        = b;
    push_heap(heap, parent);
  }
  return top_heap(heap);
}

void configure_heap(struct Heap *heap, int frequencies[SYMBOLS]) {
  for (int i = 0; i < SYMBOLS; i++) {
    if (frequencies[i]) {
      struct Node *node = (struct Node *)malloc(sizeof(struct Node));
      node->symbol      = i;
      node->frequency   = frequencies[i];
      node->right       = NULL;
      node->left        = NULL;
      push_heap(heap, node);
    }
  }
}

void archive(char *path) {
  FILE *file = fopen(path, "r");
  unsigned char data[MAX_DATA_SIZE];
  int count = fread(data, sizeof(char), MAX_DATA_SIZE, file);
  fclose(file);

  struct Heap heap         = {(struct Node **)calloc(MAX_HEAP_SIZE, sizeof(struct Node *)), 0};
  int frequencies[SYMBOLS] = {0};
  for (int i = 0; i < count; i++) {
    frequencies[data[i]]++;
  }
  configure_heap(&heap, frequencies);
  struct Node *head          = configure_huffman_tree(&heap);
  struct Code codes[SYMBOLS] = {0};
  configure_codes(head, codes, 0, 0);

  unsigned char output[MAX_DATA_SIZE] = {0};
  int i = 0, j = 0;
  for (int k = 0; k < count; k++) {
    struct Code code = codes[data[k]];
    write_code_to_result(output, code, &i, &j);
  }

  char filename[MAX_FILENAME_SIZE];
  snprintf(filename, MAX_FILENAME_SIZE, "%s.bin", path);
  FILE *archived = fopen(filename, "wb");
  fwrite(frequencies, sizeof(int), SYMBOLS, archived);
  fwrite(output, sizeof(char), j + 1, archived);
  fclose(archived);
  free_heap(&heap);
}

void put_unarchive_result(struct Node *node, unsigned char data[MAX_DATA_SIZE], int *i, int *j, unsigned char output[MAX_DATA_SIZE], int *size) {
  if (!node) return;
  if (!node->left && !node->right) {
    if (*size < MAX_DATA_SIZE - 1) output[(*size)++] = node->symbol;
    return;
  }
  int side = (data[*j] >> (*i)++) & 1;
  if (*i > 7) {
    *i = 0;
    (*j)++;
  }
  if (side) {
    put_unarchive_result(node->right, data, i, j, output, size);
  } else {
    put_unarchive_result(node->left, data, i, j, output, size);
  }
}

void unarchive(char *path) {
  FILE *file = fopen(path, "r");
  int frequencies[SYMBOLS];
  fread(frequencies, sizeof(int), SYMBOLS, file);
  unsigned char data[MAX_DATA_SIZE];
  fseek(file, sizeof(int) * SYMBOLS, SEEK_SET);
  int count = fread(data, sizeof(char), MAX_DATA_SIZE, file);
  fclose(file);

  struct Heap heap = {(struct Node **)calloc(MAX_HEAP_SIZE, sizeof(struct Node *)), 0};
  configure_heap(&heap, frequencies);
  struct Node *head                   = configure_huffman_tree(&heap);
  unsigned char output[MAX_DATA_SIZE] = {0};
  int i = 0, j = 0, size = 0;
  while (j < count - 1) {
    put_unarchive_result(head, data, &i, &j, output, &size);
  }

  char *last = strrchr(path, '.');
  if (last) *last = '\0';
  FILE *unarchived = fopen(path, "w");
  fwrite(output, sizeof(char), size, unarchived);
  fclose(unarchived);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Invalid input data\n");
    return 1;
  }
  if (!strcmp(argv[1], "archive")) {
    archive(argv[2]);
  } else if (!strcmp(argv[1], "unarchive")) {
    unarchive(argv[2]);
  } else {
    printf("There are two commands: archive and unarchive\n");
    return 1;
  }
}
