#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include <pthread.h>
#include <openssl/sha.h>

#define MAX_TRANSACTIONS 10
#define MAX_NODES 5
#define DIFFICULTY 4

//definir la strcuture de donnée qui represente le concept de transaction " sender: , recipient:, amount:
typedef struct Transaction {
    uint64_t amount;
    char sender[64];
    char recipient[64];
} Transaction;

//definir la strcture du block
typedef struct Block {
    uint32_t index; //position du bloc dans la block chain
    uint32_t nonce;// nbre aleatoire utilisé dans le proof of work
    //Les mineurs ( nœuds) modifient le nonce de manière répétée jusqu'à ce qu'ils trouvent un hash valide (qui réponde à une certaine exigence de difficulté.)
    uint32_t transactions_count;//combien de transactions sont incluses dans le bloc
    Transaction transactions[MAX_TRANSACTIONS]; //les transaction qui ont eu lieu dan sle block
    uint64_t timestamp; //date et heure de creation du bloc
    char prev_hash[65]; //le haché du bloc previous
    char hash[65]; //le haché du bloc courant
} Block;

Block blockchain[MAX_NODES][100]; //block chain est une liste de block
int lengths[MAX_NODES] = {0};

/*********************Implementation mecanisme chainage******************************/
void calculate_hash(Block* block, char* hash) { //fonction qui calcule le hashé du block en utilisant data qui est interieur
    char buffer[1024];
    memset(buffer, 0, 1024);

    for (int i = 0; i < block->transactions_count; i++) {
        Transaction* tx = &block->transactions[i];
        sprintf(buffer + strlen(buffer), "%lu%s%s", tx->amount, tx->sender, tx->recipient);
    }

    sprintf(buffer + strlen(buffer), "%u%lu%s", block->nonce, block->timestamp, block->prev_hash);

    SHA256((unsigned char*)buffer, strlen(buffer), (unsigned char*)hash);
    hash[64] = 0;
}

//construire l'arbre de Merkle à partir des transactions incluses dans un bloc donné
void build_merkle_tree(Block* block, char** merkle_tree) {
    //parcourt chaque transaction incluse dans le bloc et
    // calcule le hachage SHA-256 de chaque transaction individuelle. Ces hachages individuels sont stockés dans un tableau merkle_tree
    for (int i = 0; i < block->transactions_count; i++) {
        Transaction* tx = &block->transactions[i];
        char tx_hash[65];
        memset(tx_hash, 0, 65);
        sprintf(tx_hash, "%lu%s%s", tx->amount, tx->sender, tx->recipient);
        SHA256((unsigned char*)tx_hash, strlen(tx_hash), (unsigned char*)merkle_tree[i]);
        merkle_tree[i][64] = 0;
    }
    //combiner ces hachages pour former les nœuds de l'arbre.

    for (int i = block->transactions_count; i < 2 * block->transactions_count - 1; i++) {
        char concat[129];
        memset(concat, 0, 129);
        strcat(concat, merkle_tree[i - block->transactions_count]);
        strcat(concat, merkle_tree[i - block->transactions_count + 1]);
        SHA256((unsigned char*)concat, strlen(concat), (unsigned char*)merkle_tree[i]);
        merkle_tree[i][64] = 0;
    }
}

//3-Simuler la réplication des blocks à travers le lancement de plusieurs threads.
void* node_thread(void* arg) {
    int node_id = *(int*)arg;

    // Simuler la réception de nouveaux blocs
    Block* new_block = malloc(sizeof(Block));
    new_block->index = lengths[node_id];
    new_block->transactions_count = 1;
    new_block->transactions[0].amount = 10;
    strcpy(new_block->transactions[0].sender, "Alice");
    strcpy(new_block->transactions[0].recipient, "Bob");
    new_block->timestamp = time(NULL);

    if (lengths[node_id] == 0) {
        // Bloc génèse
        strcpy(new_block->prev_hash, "0000000000000000000000000000000000000000000000000000000000000000");
    } else {
        strcpy(new_block->prev_hash, blockchain[node_id][lengths[node_id] - 1].hash);
    }

    if (add_block(new_block, node_id)) {
        printf("Noeud %d: Nouveau bloc ajouté\n", node_id);
    } else {
        printf("Noeud %d: Erreur lors de l'ajout du bloc\n", node_id);
    }

    free(new_block);
}

int validate_transactions(Block* block) {
    // Vérifier la validité des transactions
    // (par exemple, s'assurer que l'expéditeur a suffisamment de fonds)
    // Ici, nous supposons que toutes les transactions sont valides
    return 1;
}

//Ajuster le nonce du bloc de manière itérative jusqu'à ce qu'un hash valide soit trouvé
int proof_of_work(Block* block, int difficulty) {
    char target[65];
    memset(target, 0, 65);
    for (int i = 0; i < difficulty; i++) {
        target[i] = '0';
    }

    char hash[65];
    uint32_t nonce = 0;

    do {
        block->nonce = nonce;
        calculate_hash(block, hash);
        nonce++;
    } while (memcmp(hash, target, difficulty / 8) > 0);

    strcpy(block->hash, hash);
    return 1; //Une fois le hash valide trouvé, il est stocké dans le bloc et la fonction retourne 1 pour indiquer le succès du minage du bloc.
}

//mecanisme ajout d'un block dans la blockchain
int add_block(Block* block, int node_id) {
    if (!validate_transactions(block)) {
        return 0; // Les transactions ne sont pas valides je ne peux pas l ajouter
    }

    if (!proof_of_work(block, DIFFICULTY)) {
        return 0; // La preuve de travail a échoué
    }

    // si tout est valide, Ajouter le bloc à la blockchain
    blockchain[node_id][lengths[node_id]] = *block;
    lengths[node_id]++;

    return 1;
}

void print_blockchain(int node_id) {
    printf("Blockchain du noeud %d:\n", node_id);
    for (int i = 0; i < lengths[node_id]; i++) {
        Block* block = &blockchain[node_id][i];
        printf("Block %u:\n", block->index);
        printf("  Hash: %s\n", block->hash);
        printf("  Previous Hash: %s\n", block->prev_hash);
        printf("  Timestamp: %lu\n", block->timestamp);
        printf("  Nonce: %u\n", block->nonce);
        printf("  Transactions:\n");
        for (int j = 0; j < block->transactions_count; j++) {
            Transaction* tx = &block->transactions[j];
            printf("    %s -> %s: %lu\n", tx->sender, tx->recipient, tx->amount);
        }
        printf("\n");
    }
}

int main() {
    pthread_t threads[MAX_NODES];
    int node_ids[MAX_NODES];

    for (int i = 0; i < MAX_NODES; i++) {
        node_ids[i] = i;
        pthread_create(&threads[i], NULL, node_thread, &node_ids[i]);
    }

    for (int i = 0; i < MAX_NODES; i++) {
        pthread_join(threads[i], NULL);
    }

    // Afficher la blockchain de chaque nœud
    for (int i = 0; i < MAX_NODES; i++) {
        print_blockchain(i);
    }

    return 0;
}
