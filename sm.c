#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


#ifdef _WIN32
    #define CLEAR_SCREEN "cls"
    #include <windows.h>
#else
    #define CLEAR_SCREEN "clear"
#endif

void setupConsole() {
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    #else
        setlocale(LC_ALL, "ru_RU.UTF-8");
    #endif
}

// Структуры данных
typedef struct Customer {
    char name;
    int service_time;
    int check_sum;
} Customer;

typedef struct QueueNode {
    Customer customer;
    struct QueueNode* next;
} QueueNode;

typedef struct Queue {
    QueueNode* front;
    QueueNode* rear;
    int length;
} Queue;

typedef struct Cashier {
    int is_active;
    int served_customers;
    int total_check_sum;
    Queue queue;
} Cashier;

// Глобальные параметры из файла
int MAX_CUSTOMER_TIME = 0;
int MAX_CASHIER_QUEUE = 0;
int MAX_CASHIERS = 0;
int MAX_NEXT_CUSTOMERS = 0;
int MAX_CUSTOMER_CHECK = 0;

// Глобальные переменные симуляции
Cashier* cashiers = NULL;
int current_time = 0;
int total_customers_served = 0;
int total_check_sum_all = 0;

// Прототипы функций
void load_settings();
void init_queue(Queue* q);
void enqueue(Queue* q, Customer customer);
Customer dequeue(Queue* q);
void init_cashiers();
Customer generate_customer();
void generate_next_customers(Customer* next_customers, int* next_count);
int get_active_cashiers_count();
int get_total_customers_in_queues();
int find_cashier_with_min_queue();
void open_new_cashier();
void close_cashier_if_needed();
void distribute_customers(Customer* customers, int count);
void process_cashiers();
void print_interface(Customer* next_customers, int next_count); // Добавлен прототип
void run_simulation();
void cleanup();

// Загрузка параметров из файла
void load_settings() {
    FILE* file = fopen("settings.txt", "r");
    if (file == NULL) {
        printf("Ошибка: файл settings.txt не найден.\n");
        exit(1);
    }

    char line[100];
    while (fgets(line, sizeof(line), file)) {
        char key[50];
        int value;
        if (sscanf(line, "%[^=]=%d", key, &value) == 2) {
            if (strcmp(key, "MAX_CUSTOMER_TIME") == 0) MAX_CUSTOMER_TIME = value;
            else if (strcmp(key, "MAX_CASHIER_QUEUE") == 0) MAX_CASHIER_QUEUE = value;
            else if (strcmp(key, "MAX_CASHIERS") == 0) MAX_CASHIERS = value;
            else if (strcmp(key, "MAX_NEXT_CUSTOMERS") == 0) MAX_NEXT_CUSTOMERS = value;
            else if (strcmp(key, "MAX_CUSTOMER_CHECK") == 0) MAX_CUSTOMER_CHECK = value;
        }
    }
    fclose(file);

    // Проверка всех параметров
    if (MAX_CUSTOMER_TIME == 0 || MAX_CASHIER_QUEUE == 0 || 
        MAX_CASHIERS == 0 || MAX_NEXT_CUSTOMERS == 0 || 
        MAX_CUSTOMER_CHECK == 0) {
        printf("Ошибка: не все параметры заданы в settings.txt\n");
        exit(1);
    }
}

// Инициализация очереди
void init_queue(Queue* q) {
    q->front = q->rear = NULL;
    q->length = 0;
}

// Добавление в очередь
void enqueue(Queue* q, Customer customer) {
    if (q->length >= MAX_CASHIER_QUEUE) return;
    
    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    newNode->customer = customer;
    newNode->next = NULL;
    
    if (q->rear == NULL) {
        q->front = q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
    q->length++;
}

// Удаление из очереди
Customer dequeue(Queue* q) {
    Customer empty_customer = {' ', 0, 0};
    if (q->front == NULL) return empty_customer;
    
    QueueNode* temp = q->front;
    Customer customer = temp->customer;
    q->front = q->front->next;
    
    if (q->front == NULL) q->rear = NULL;
    
    free(temp);
    q->length--;
    return customer;
}

// Инициализация всех касс
void init_cashiers() {
    cashiers = (Cashier*)malloc(MAX_CASHIERS * sizeof(Cashier));
    for (int i = 0; i < MAX_CASHIERS; i++) {
        cashiers[i].is_active = 0;
        cashiers[i].served_customers = 0;
        cashiers[i].total_check_sum = 0;
        init_queue(&cashiers[i].queue);
    }
    cashiers[0].is_active = 1; // Первая касса всегда активна
}

// Генерация случайного посетителя
Customer generate_customer() {
    Customer customer;
    customer.name = 'a' + rand() % 26;
    customer.service_time = 1 + rand() % MAX_CUSTOMER_TIME;
    customer.check_sum = 1 + rand() % MAX_CUSTOMER_CHECK;
    return customer;
}

// Генерация посетителей для следующего шага
void generate_next_customers(Customer* next_customers, int* next_count) {
    *next_count = rand() % (MAX_NEXT_CUSTOMERS + 1);
    for (int i = 0; i < *next_count; i++) {
        next_customers[i] = generate_customer();
    }
}

// Количество активных касс
int get_active_cashiers_count() {
    int count = 0;
    for (int i = 0; i < MAX_CASHIERS; i++) {
        if (cashiers[i].is_active) count++;
    }
    return count;
}

// Общее количество людей в очередях
int get_total_customers_in_queues() {
    int total = 0;
    for (int i = 0; i < MAX_CASHIERS; i++) {
        if (cashiers[i].is_active) total += cashiers[i].queue.length;
    }
    return total;
}

// Касса с минимальной очередью
int find_cashier_with_min_queue() {
    int min_index = -1;
    int min_length = MAX_CASHIER_QUEUE + 1;
    
    for (int i = 0; i < MAX_CASHIERS; i++) {
        if (cashiers[i].is_active && cashiers[i].queue.length < min_length) {
            min_length = cashiers[i].queue.length;
            min_index = i;
        }
    }
    return min_index;
}

// Открытие новой кассы
void open_new_cashier() {
    for (int i = 0; i < MAX_CASHIERS; i++) {
        if (!cashiers[i].is_active) {
            cashiers[i].is_active = 1;
            return;
        }
    }
}

// Закрытие кассы с максимальным количеством обслуженных
void close_cashier_if_needed() {
    if (get_total_customers_in_queues() > 0) return;
    
    int max_served = -1;
    int max_index = -1;
    
    for (int i = 1; i < MAX_CASHIERS; i++) {
        if (cashiers[i].is_active && cashiers[i].served_customers > max_served) {
            max_served = cashiers[i].served_customers;
            max_index = i;
        }
    }
    
    if (max_index != -1 && get_active_cashiers_count() > 1) {
        cashiers[max_index].is_active = 0;
    }
}

// Распределение посетителей по кассам
void distribute_customers(Customer* customers, int count) {
    for (int i = 0; i < count; i++) {
        int cashier_index = find_cashier_with_min_queue();
        
        // Если нет места, открываем новую кассу
        if (cashier_index == -1 || cashiers[cashier_index].queue.length >= MAX_CASHIER_QUEUE) {
            open_new_cashier();
            cashier_index = find_cashier_with_min_queue();
            
            // Если все кассы заполнены - Game Over
            if (cashier_index == -1) {
                printf("Game Over: невозможно разместить всех посетителей!\n");
                print_interface(NULL, 0);
                exit(0);
            }
        }
        enqueue(&cashiers[cashier_index].queue, customers[i]);
    }
}

// Обработка обслуживания за одну секунду
void process_cashiers() {
    for (int i = 0; i < MAX_CASHIERS; i++) {
        if (cashiers[i].is_active && cashiers[i].queue.front != NULL) {
            // Уменьшаем время обслуживания первого в очереди
            cashiers[i].queue.front->customer.service_time--;
            
            // Если время истекло - удаляем посетителя
            if (cashiers[i].queue.front->customer.service_time == 0) {
                Customer served = dequeue(&cashiers[i].queue);
                cashiers[i].served_customers++;
                cashiers[i].total_check_sum += served.check_sum;
                total_customers_served++;
                total_check_sum_all += served.check_sum;
            }
        }
    }
}

// Вывод интерфейса
void print_interface(Customer* next_customers, int next_count) {
    system(CLEAR_SCREEN);
    printf("Супермаркет \"Реми\". Система моделирования очередей.\n\n");
    
    // Номера касс
    printf("  ");
    for (int i = 0; i < MAX_CASHIERS; i++) printf("%3d ", i + 1);
    printf("\n");
    
    // Обслужено посетителей
    printf("  ");
    for (int i = 0; i < MAX_CASHIERS; i++) printf("%3d ", cashiers[i].served_customers);
    printf("\n");
    
    // Статус кассы
    printf("  ");
    for (int i = 0; i < MAX_CASHIERS; i++) printf("  %c ", cashiers[i].is_active ? '+' : '-');
    printf("\n");
    
    // Очереди на кассах
    for (int row = 0; row < MAX_CASHIER_QUEUE; row++) {
        printf("  ");
        for (int i = 0; i < MAX_CASHIERS; i++) {
            if (cashiers[i].is_active) {
                QueueNode* current = cashiers[i].queue.front;
                int j = 0;
                while (current != NULL && j < row) {
                    current = current->next;
                    j++;
                }
                
                if (current != NULL && j == row) {
                    printf(" %c%d ", current->customer.name, current->customer.service_time);
                } else {
                    printf(" || ");
                }
            } else {
                printf(" || ");
            }
        }
        printf("\n");
    }
    
    // Статистика
    printf("\nВремя: %d\n", current_time);
    
    printf("Следующие посетители: ");
    if (next_count == 0) printf("нет");
    else for (int i = 0; i < next_count; i++) printf("%c%d ", next_customers[i].name, next_customers[i].service_time);
    printf("\n");
    
    printf("Человек в очередях: %d\n", get_total_customers_in_queues());
    printf("Касс работает: %d из %d\n", get_active_cashiers_count(), MAX_CASHIERS);
    printf("Всего обслужено: %d\n", total_customers_served);
    printf("Сумма покупок: %d\n", total_check_sum_all);
    printf("Допустимая очередь на кассу: %d\n", MAX_CASHIER_QUEUE);
}

// Основная функция симуляции
void run_simulation() {
    srand(time(NULL));
    load_settings();
    init_cashiers();
    
    Customer next_customers[MAX_NEXT_CUSTOMERS];
    int next_count = 0;
    
    generate_next_customers(next_customers, &next_count);
    
    while (1) {
        print_interface(next_customers, next_count);
        process_cashiers();
        distribute_customers(next_customers, next_count);
        close_cashier_if_needed();
        generate_next_customers(next_customers, &next_count);
        current_time++;
        sleep(1);
    }
}

// Освобождение памяти
void cleanup() {
    if (cashiers != NULL) {
        for (int i = 0; i < MAX_CASHIERS; i++) {
            while (cashiers[i].queue.front != NULL) dequeue(&cashiers[i].queue);
        }
        free(cashiers);
    }
}

int main() {
    #ifdef _WIN32
        SetConsoleOutputCP(1251);
        SetConsoleCP(1251);
    #endif
    
    atexit(cleanup);
    run_simulation();
    return 0;
}