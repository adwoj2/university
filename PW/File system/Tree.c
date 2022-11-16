#include "HashMap.h"
#include "path_utils.h"
#include "Tree.h"
#include "err.h"
#include "rwlock.h"
#include <errno.h>


#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define EINSOURCE -1; /*kod błędu pojawiający się gdy operacja move próbuje
przesunąć pewien folder do jego wnętrza.*/

typedef struct Tree {
    HashMap* map;
    pthread_mutex_t lock;
    struct readwrite* rwlock;
    Tree* parent;
} Tree;

Tree* tree_new() {
    Tree* tree = malloc(sizeof(Tree));
    tree->map = hmap_new();

    if ((pthread_mutex_init(&tree->lock, 0)) != 0)
        syserr ("mutex init failed");

    tree->rwlock = malloc (sizeof(struct readwrite));
    init(tree->rwlock);
    tree->parent = NULL;
    return tree;
}

//Zwalnia wszystkie zajęte rwlocki na ścieżce
static void tree_exit_subfolder(Tree* tree) {
    reader_unlock(tree->rwlock);
    if (tree->parent != NULL) 
        tree_exit_subfolder(tree->parent);
}
/*operacja rekurencyjnie zwraca podfolder okreslony ścieżką path i zajmuje 
wszystkie węzły po drodze przez czytelników */
static Tree* tree_go_to_subfolder(Tree* tree, const char* path, bool not_reserving) {

    char* component = malloc(sizeof(char) * (MAX_FOLDER_NAME_LENGTH + 1));
    const char* subpath; 
    subpath = split_path(path, component);

    if (subpath == NULL) {
        free(component);
        return tree;
    }
    if (!not_reserving) {
        reader_lock(tree->rwlock);
    } 

    Tree* new_tree = hmap_get(tree->map, component);

    free(component);
    if (new_tree == NULL) {
        if (!not_reserving) {
            tree_exit_subfolder (tree);
        }
        return NULL;
    }
    return tree_go_to_subfolder(new_tree, subpath, not_reserving);
}
//funkcja wypisująca wszystkie pliki w danym węźle -- czytelnik
static char* subtree_list(Tree* tree) {
    reader_lock(tree->rwlock);
    char* list = make_map_contents_string(tree->map); 
    reader_unlock(tree->rwlock);
    return list;
}


char* tree_list(Tree* tree, const char* path) {
    
    if (!is_path_valid(path))
        return NULL;

    Tree* subfolder = tree_go_to_subfolder(tree, path, false);

    if(subfolder == NULL) 
        return NULL;

    char* res = subtree_list(subfolder);
    if (subfolder->parent != NULL)
        tree_exit_subfolder(subfolder->parent);
    return res;
}
//funkcja zwalniająca dany węzeł
static void subtree_free(Tree* tree) {
    hmap_free(tree->map);
    destroy(tree->rwlock);
    free(tree->rwlock);
    free(tree);
}

void tree_free(Tree* tree) {
    int x = hmap_size(tree->map);
    if (x > 0) {
        const char** keys = make_map_contents_array(tree->map);
        for (int i = 0; i < x; i++){
            tree_free(hmap_get(tree->map, keys[i]));
        }
        free(keys);
    }

    hmap_free(tree->map);
    destroy(tree->rwlock);
    free(tree->rwlock);
    free(tree);
}
//funkcja tworząca nowy plik w danym węźle -- pisarz
static int subtree_create(Tree* tree, char* name) {
    Tree* new_folder = tree_new();
    new_folder->parent = tree;

    writer_lock(tree->rwlock);
    if (!hmap_insert(tree->map, name, new_folder)) {
        writer_unlock(tree->rwlock);
        destroy(new_folder->rwlock);
        free(new_folder->rwlock);
        hmap_free(new_folder->map);
        free(new_folder);

        return EEXIST; 
    }
    writer_unlock(tree->rwlock);
    return 0;
}

int tree_create(Tree* tree, const char* path) {
    if (!is_path_valid(path))
        return EINVAL; 
    char* name = malloc(sizeof(char) * (MAX_FOLDER_NAME_LENGTH + 1));

    char* path_to_parent;
    path_to_parent = make_path_to_parent(path, name);

    if (path_to_parent == NULL) {
        free(name);
        return EEXIST;
    }

    Tree* subfolder;
    subfolder = tree_go_to_subfolder(tree, path_to_parent, false);
    if (subfolder == NULL) {
        free(path_to_parent);
        free(name);
        return ENOENT;
    }

    int res = subtree_create(subfolder, name);
    if (subfolder->parent != NULL)
        tree_exit_subfolder(subfolder->parent);
    free(path_to_parent);
    free(name);
    return res;
}
//funkcja tworząca usuwająca plik w danym węźle -- pisarz
static int subtree_remove(Tree* tree, char* name) {

    writer_lock(tree->rwlock);
    Tree* folder = hmap_get(tree->map, name);

    if (folder == NULL){
        writer_unlock(tree->rwlock);
        return ENOENT;
    }

    if (hmap_size(folder->map) > 0) {
        writer_unlock(tree->rwlock);
        return ENOTEMPTY;
    }    

    subtree_free(folder);
    hmap_remove(tree->map, name);
    writer_unlock(tree->rwlock);
    return 0;
}

int tree_remove(Tree* tree, const char* path) {
    
    if (strcmp(path, "/") == 0)
        return EBUSY;
    if (!is_path_valid(path))
        return EINVAL; 
    char* name = malloc(sizeof(char) * (MAX_FOLDER_NAME_LENGTH + 1));
    char* path_to_parent;
    path_to_parent = make_path_to_parent(path, name);

    Tree* subfolder = tree_go_to_subfolder(tree, path_to_parent, false);

    if (subfolder == NULL) {
        free(path_to_parent);
        free(name);
        return ENOENT;
    }

    int res = subtree_remove(subfolder, name);
    if (subfolder->parent != NULL)
        tree_exit_subfolder(subfolder->parent);

    free(path_to_parent);
    free(name);
    return res;
}

int tree_move(Tree* tree, const char* source, const char* target) {
    if (!is_path_valid(source)) 
        return EINVAL; 

    if (!is_path_valid(target)) 
        return EINVAL; 

    if (strcmp(source, "/") == 0)     
        return EBUSY;

    if ((strcmp(target, source) != 0) && strncmp(source, target, strlen(source)) == 0) {
        return EINSOURCE;
    }

    char* path_to_source_parent;
    char* name = malloc(sizeof(char) * (MAX_FOLDER_NAME_LENGTH + 1));
    path_to_source_parent = make_path_to_parent(source, name);

    char* path_to_target_parent;
    char* new_name = malloc(sizeof(char) * (MAX_FOLDER_NAME_LENGTH + 1));
    path_to_target_parent = make_path_to_parent(target, new_name);

    if (path_to_target_parent == NULL){
        free(name);
        free(new_name);
        free(path_to_source_parent);
        free(path_to_target_parent);
        return EEXIST;
    }

    /*Ddopuszczanie tylko i wyłącznie jednego procesu wykonującego move oraz 
    zapewnienie że żadna inna operacja nie wykonuje się w tym czasie. Jest
    to bardzo słaba współbieżność jednak jedynie taką udało mi się napisać
    unikając zakleszczeń i zagłodzeń więc zdecydowałem się ją przesłać.
    */
    writer_lock(tree->rwlock);

    Tree* source_parent = tree_go_to_subfolder(tree, path_to_source_parent, true);
    Tree* target_parent = tree_go_to_subfolder(tree, path_to_target_parent, true);

    if (source_parent == NULL) {

        writer_unlock(tree->rwlock);
        free(path_to_source_parent);
        free(new_name);
        free(name);
        
        free(path_to_target_parent);
        return ENOENT;
    
    }

   if (target_parent == NULL) {
        writer_unlock(tree->rwlock);
        
        free(path_to_target_parent);
        free(path_to_source_parent);
        free(new_name);
        free(name);
        return ENOENT;
    }

    Tree* object = hmap_get(source_parent->map, name);
 
    if (object == NULL) {
        writer_unlock(tree->rwlock);

        
        free(path_to_target_parent);
        free(path_to_source_parent);
        free(new_name);
        free(name);

        return ENOENT;
    }

    if (strcmp(target, source) == 0) {
        writer_unlock(tree->rwlock);
        free(path_to_target_parent);
        free(path_to_source_parent);
        free(new_name);
        free(name);

        return 0;
    }
    
    if (!hmap_insert(target_parent->map, new_name, object)) {
        writer_unlock(tree->rwlock);
        free(path_to_target_parent);
        free(path_to_source_parent);
        free(new_name);
        free(name);

        return EEXIST;
    }

    hmap_remove(source_parent->map, name);
    object->parent = target_parent;
    writer_unlock(tree->rwlock);

    free(path_to_target_parent);
    free(path_to_source_parent);
    free(new_name);
    free(name);
    return 0;
}
