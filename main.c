#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_HEAP_SIZE      1024
#define MAX_METADATA_SIZE  1024
#define MAX_DATA_SIZE      1048576
#define MAX_PATH_SIZE      256
#define MAX_EXTENSION_SIZE 16
#define SYMBOLS            256

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

struct SymbolPair {
  unsigned char symbol;
  int frequency;
};

#pragma pack(push, 1)
struct Metadata {
  int path_size;
  int frequencies_size;
  int data_size;
};
#pragma pack(pop)

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

void write_code_to_result(unsigned char result[MAX_DATA_SIZE], struct Code code, int *i, int *j) {
  int k = 0;
  while (k < code.size) {
    if (*i > 7) {
      *i = 0;
      (*j)++;
    }
    result[*j] |= ((code.code >> (code.size - k++ - 1)) & 1) << (*i)++;
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

void get_file_extension(char *path, char *extension) {
  char *extension_start = strrchr(path, '.');
  char *dir_start       = strrchr(path, '/');
  if (!extension_start || (dir_start && dir_start > extension_start)) {
    extension[0] = '\0';
    return;
  }
  strncpy(extension, extension_start, strlen(extension_start));
}

void archive_file(char *path, FILE *output) {
  FILE *file                        = fopen(path, "r");
  unsigned char data[MAX_DATA_SIZE] = {0};
  int count                         = fread(data, sizeof(data[0]), sizeof(data), file);
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

  unsigned char result[MAX_DATA_SIZE] = {0};
  int i = 0, j = 0;
  for (int k = 0; k < count; k++) {
    struct Code code = codes[data[k]];
    write_code_to_result(result, code, &i, &j);
  }

  struct SymbolPair pairs[SYMBOLS] = {0};
  int pairs_count                  = 0;
  for (int i = 0; i < SYMBOLS; i++) {
    if (frequencies[i]) pairs[pairs_count++] = (struct SymbolPair){i, frequencies[i]};
  }

  struct Metadata metadata = {
      strlen(path),
      pairs_count,
      j + 1,
  };
  fwrite(&metadata, sizeof(struct Metadata), 1, output);
  fwrite(path, sizeof(path[0]), metadata.path_size, output);
  fwrite(pairs, sizeof(struct SymbolPair), metadata.frequencies_size, output);
  fwrite(result, sizeof(result[0]), metadata.data_size, output);
  free_heap(&heap);
}

void archive_dir(char *path, FILE *output) {
  DIR *dir = opendir(path);
  if (!dir) {
    printf("Failed to read directory\n");
    exit(1);
  }
  struct dirent *ent;
  while ((ent = readdir(dir))) {
    if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
    char ent_path[MAX_PATH_SIZE];
    snprintf(ent_path, sizeof(ent_path), "%s/%s", path, ent->d_name);
    struct stat ent_stat;
    stat(ent_path, &ent_stat);
    if (S_ISREG(ent_stat.st_mode)) {
      archive_file(ent_path, output);
      printf("%s\n", ent_path);
    } else if (S_ISDIR(ent_stat.st_mode)) {
      archive_dir(ent_path, output);
    }
  }
  closedir(dir);
}

void unarchive(char *path) {
  FILE *file = fopen(path, "r");
  while (1) {
    struct Metadata metadata         = {0};
    char file_path[MAX_PATH_SIZE]    = {0};
    struct SymbolPair pairs[SYMBOLS] = {0};
    fread(&metadata, sizeof(struct Metadata), 1, file);
    if (!metadata.path_size || !metadata.frequencies_size || !metadata.data_size) break;
    fread(file_path, sizeof(file_path[0]), metadata.path_size, file);
    fread(pairs, sizeof(struct SymbolPair), metadata.frequencies_size, file);
    int frequencies[SYMBOLS] = {0};
    for (int i = 0; i < metadata.frequencies_size; i++) {
      frequencies[pairs[i].symbol] = pairs[i].frequency;
    }

    unsigned char data[MAX_DATA_SIZE] = {0};
    int data_readed_count             = fread(data, sizeof(data[0]), metadata.data_size, file);

    struct Heap heap = {(struct Node **)calloc(MAX_HEAP_SIZE, sizeof(struct Node *)), 0};
    configure_heap(&heap, frequencies);
    struct Node *head                   = configure_huffman_tree(&heap);
    unsigned char result[MAX_DATA_SIZE] = {0};
    int i = 0, j = 0, size = 0;
    while (j < data_readed_count - 1) {
      put_unarchive_result(head, data, &i, &j, result, &size);
    }

    for (int i = 0; i < metadata.path_size; i++) {
      if (file_path[i] == '/') {
        char temp    = file_path[i];
        file_path[i] = '\0';
        mkdir(file_path, 0777);
        file_path[i] = temp;
      }
    }
    FILE *output = fopen(file_path, "w");
    if (!output) {
      printf("Failed to open output file\n");
      continue;
    }
    fwrite(result, sizeof(result[0]), size, output);
    fclose(output);
  }
  fclose(file);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Invalid input data\n");
    return 1;
  }

  char *path = argv[2];
  char *mode = argv[1];
  struct stat path_stat;
  stat(path, &path_stat);

  if (!strcmp(mode, "archive")) {
    char where[MAX_PATH_SIZE];
    snprintf(where, sizeof(where), "%s.bin", path);
    FILE *output = fopen(where, "wb");
    if (!output) {
      printf("Failed to open output file\n");
      return 1;
    }
    if (S_ISREG(path_stat.st_mode)) {
      archive_file(path, output);
    } else if (S_ISDIR(path_stat.st_mode)) {
      archive_dir(path, output);
    }
    fclose(output);
  } else if (!strcmp(mode, "unarchive")) {
    char extension[MAX_EXTENSION_SIZE];
    get_file_extension(path, extension);
    if (!strlen(extension) || strcmp(extension, ".bin")) {
      printf("This is not an archive\n");
      return 1;
    }
    unarchive(path);
  } else {
    printf("There are two commands: archive and unarchive\n");
    return 1;
  }
}
