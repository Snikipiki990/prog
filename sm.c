#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

// Структуры данных
struct Customer {
    char name;
    int time;
    int money;
};

struct Node {
    struct Customer person;
    struct Node* next;
};

struct Queue {
    struct Node* first;
    struct Node* last;
    int size;
};

struct Cashier {
    int work;
    int done;
    int total_money;
    struct Queue line;
};

// Глобальные переменные
int MAX_TIME = 0;
int MAX_QUEUE = 0;
int MAX_CASH = 0;
int MAX_NEW = 0;
int MAX_MONEY = 0;

struct Cashier* all_cashiers = NULL;
int seconds = 0;
int all_done = 0;
int all_money = 0;

// Функции
void read_settings() {
    FILE* f = fopen("settings.txt", "r");
    if (!f) {
        printf("Error: settings.txt not found!\n");
        exit(1);
    }
    
    char line[100];
    while (fgets(line, 100, f)) {
        char* eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char* name = line;
            int value = atoi(eq + 1);
            
            if (strcmp(name, "MAX_CUSTOMER_TIME") == 0) MAX_TIME = value;
            else if (strcmp(name, "MAX_CASHIER_QUEUE") == 0) MAX_QUEUE = value;
            else if (strcmp(name, "MAX_CASHIERS") == 0) MAX_CASH = value;
            else if (strcmp(name, "MAX_NEXT_CUSTOMERS") == 0) MAX_NEW = value;
            else if (strcmp(name, "MAX_CUSTOMER_CHECK") == 0) MAX_MONEY = value;
        }
    }
    fclose(f);
    
    if (MAX_TIME == 0 || MAX_QUEUE == 0 || MAX_CASH == 0 || MAX_NEW == 0 || MAX_MONEY == 0) {
        printf("Error in settings file!\n");
        exit(1);
    }
}

void make_empty_queue(struct Queue* q) {
    q->first = NULL;
    q->last = NULL;
    q->size = 0;
}

void add_to_queue(struct Queue* q, struct Customer c) {
    if (q->size >= MAX_QUEUE) return;
    
    struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
    new_node->person = c;
    new_node->next = NULL;
    
    if (q->last == NULL) {
        q->first = new_node;
        q->last = new_node;
    } else {
        q->last->next = new_node;
        q->last = new_node;
    }
    q->size++;
}

struct Customer take_from_queue(struct Queue* q) {
    struct Customer empty = {' ', 0, 0};
    if (q->first == NULL) return empty;
    
    struct Node* temp = q->first;
    struct Customer c = temp->person;
    q->first = q->first->next;
    
    if (q->first == NULL) q->last = NULL;
    
    free(temp);
    q->size--;
    return c;
}

void prepare_cashiers() {
    all_cashiers = (struct Cashier*)malloc(MAX_CASH * sizeof(struct Cashier));
    for (int i = 0; i < MAX_CASH; i++) {
        all_cashiers[i].work = 0;
        all_cashiers[i].done = 0;
        all_cashiers[i].total_money = 0;
        make_empty_queue(&all_cashiers[i].line);
    }
    all_cashiers[0].work = 1; // первая касса работает
}

struct Customer make_customer() {
    struct Customer c;
    c.name = 'a' + rand() % 26;
    c.time = 1 + rand() % MAX_TIME;
    c.money = 1 + rand() % MAX_MONEY;
    return c;
}

int working_cashiers() {
    int count = 0;
    for (int i = 0; i < MAX_CASH; i++) {
        if (all_cashiers[i].work) count++;
    }
    return count;
}

int people_in_lines() {
    int total = 0;
    for (int i = 0; i < MAX_CASH; i++) {
        if (all_cashiers[i].work) total += all_cashiers[i].line.size;
    }
    return total;
}

// ИСПРАВЛЕННАЯ ФУНКЦИЯ: Проверяем, нужно ли включить новую кассу
int need_new_cashier() {
    // Если все кассы уже работают, не нужно включать новую
    if (working_cashiers() == MAX_CASH) return 0;
    
    // Проверяем все работающие кассы
    for (int i = 0; i < MAX_CASH; i++) {
        if (all_cashiers[i].work) {
            // Если хоть одна работающая касса НЕ заполнена полностью, 
            // значит новая касса пока не нужна
            if (all_cashiers[i].line.size < MAX_QUEUE) {
                return 0; // Не нужно включать
            }
        }
    }
    
    // Все работающие кассы заполнены до предела
    return 1; // Нужно включить новую кассу
}

// ИСПРАВЛЕННАЯ ФУНКЦИЯ: Включаем новую кассу (самую левую неактивную)
void turn_on_new_cashier() {
    for (int i = 0; i < MAX_CASH; i++) {
        if (!all_cashiers[i].work) {
            all_cashiers[i].work = 1;
            printf("\nВключена касса %d!\n", i+1);
            return;
        }
    }
}

void check_customers() {
    for (int i = 0; i < MAX_CASH; i++) {
        if (all_cashiers[i].work && all_cashiers[i].line.first != NULL) {
            all_cashiers[i].line.first->person.time--;
            
            if (all_cashiers[i].line.first->person.time == 0) {
                struct Customer done = take_from_queue(&all_cashiers[i].line);
                all_cashiers[i].done++;
                all_cashiers[i].total_money += done.money;
                all_done++;
                all_money += done.money;
            }
        }
    }
}

// Расставляем покупателей по кассам
int place_customers(struct Customer* new_people, int count) {
    // Сначала проверяем, нужно ли включить новую кассу
    if (count > 0 && need_new_cashier()) {
        turn_on_new_cashier();
    }
    
    // Теперь расставляем покупателей
    for (int i = 0; i < count; i++) {
        int placed = 0; // флаг, что покупатель размещен
        
        // Сначала пытаемся разместить в работающие кассы слева-направо
        for (int j = 0; j < MAX_CASH; j++) {
            if (all_cashiers[j].work && all_cashiers[j].line.size < MAX_QUEUE) {
                add_to_queue(&all_cashiers[j].line, new_people[i]);
                placed = 1;
                break;
            }
        }
        
        // Если не удалось разместить (все работающие кассы полные)
        if (!placed) {
            // Пытаемся включить новую кассу
            for (int j = 0; j < MAX_CASH; j++) {
                if (!all_cashiers[j].work) {
                    all_cashiers[j].work = 1;
                    add_to_queue(&all_cashiers[j].line, new_people[i]);
                    placed = 1;
                    printf("\nВключена касса %d для нового покупателя!\n", j+1);
                    break;
                }
            }
        }
        
        // Если все кассы работают и все полные - Game Over
        if (!placed) {
            return 0; // Game Over
        }
    }
    return 1; // Все размещено
}

void turn_off_extra() {
    if (people_in_lines() > 0) return;
    
    // Не выключаем первую кассу
    if (working_cashiers() <= 1) return;
    
    int max_done = -1;
    int idx = -1;
    
    // Ищем кассу с максимальным количеством обслуженных (кроме первой)
    for (int i = 1; i < MAX_CASH; i++) {
        if (all_cashiers[i].work && all_cashiers[i].done > max_done) {
            max_done = all_cashiers[i].done;
            idx = i;
        }
    }
    
    // Выключаем найденную кассу
    if (idx != -1) {
        all_cashiers[idx].work = 0;
        printf("\nВыключена касса %d (обслужила %d покупателей)\n", idx+1, max_done);
    }
}

void clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void show_screen(struct Customer* next_people, int next_count) {
    clear_screen();
    printf("Супермаркет \"Реми\". Система моделирования очередей.\n\n");
    
    printf("  ");
    for (int i = 0; i < MAX_CASH; i++) {
        printf("%3d ", i+1);
    }
    printf("\n");
    
    printf("  ");
    for (int i = 0; i < MAX_CASH; i++) {
        printf("%3d ", all_cashiers[i].done);
    }
    printf("\n");
    
    printf("  ");
    for (int i = 0; i < MAX_CASH; i++) {
        if (all_cashiers[i].work) printf("  + ");
        else printf("  - ");
    }
    printf("\n");
    
    for (int row = 0; row < MAX_QUEUE; row++) {
        printf("  ");
        for (int i = 0; i < MAX_CASH; i++) {
            if (all_cashiers[i].work) {
                struct Node* cur = all_cashiers[i].line.first;
                int pos = 0;
                while (cur != NULL && pos < row) {
                    cur = cur->next;
                    pos++;
                }
                
                if (cur != NULL && pos == row) {
                    printf(" %c%d ", cur->person.name, cur->person.time);
                } else {
                    printf(" || ");
                }
            } else {
                printf(" || ");
            }
        }
        printf("\n");
    }
    
    printf("\nВремя: %d\n", seconds);
    
    printf("Следующие посетители: ");
    if (next_count == 0) printf("нет");
    else for (int i = 0; i < next_count; i++) {
        printf("%c%d ", next_people[i].name, next_people[i].time);
    }
    printf("\n");
    
    printf("Человек в очередях: %d\n", people_in_lines());
    printf("Касс работает: %d из %d\n", working_cashiers(), MAX_CASH);
    printf("Всего обслужено: %d\n", all_done);
    printf("Сумма покупок: %d\n", all_money);
    printf("Допустимая очередь на кассу: %d\n", MAX_QUEUE);
}

int main() {
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    #endif
    
    srand(time(NULL));
    read_settings();
    prepare_cashiers();
    
    struct Customer next[MAX_NEW];
    int next_num = rand() % (MAX_NEW + 1);
    for (int i = 0; i < next_num; i++) {
        next[i] = make_customer();
    }
    
    while (1) {
        show_screen(next, next_num);
        
        // Пауза для просмотра
        #ifdef _WIN32
            Sleep(500);
        #else
            usleep(500000);
        #endif
        
        check_customers();
        
        int ok = place_customers(next, next_num);
        if (!ok) {
            printf("\n\nGAME OVER! Все кассы заполнены!\n");
            printf("Нажмите Enter...");
            getchar();
            break;
        }
        
        turn_off_extra();
        
        next_num = rand() % (MAX_NEW + 1);
        for (int i = 0; i < next_num; i++) {
            next[i] = make_customer();
        }
        
        seconds++;
        
        // Ждем оставшееся время
        #ifdef _WIN32
            Sleep(500);
        #else
            usleep(500000);
        #endif
    }
    
    // Освобождаем память
    if (all_cashiers != NULL) {
        for (int i = 0; i < MAX_CASH; i++) {
            while (all_cashiers[i].line.first != NULL) {
                take_from_queue(&all_cashiers[i].line);
            }
        }
        free(all_cashiers);
    }
    
    return 0;
}