#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>


#define MAX_STUDENT 100 // Student count at library
#define CLASSRO0M 10    // Classroom count at library
#define WORKTABLE 4     // Each classroom has 4 worktable

/* Defined color printing globals*/
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"


void *student(void *id);

void *room_keeper(void *id);

void monitorize_current_event();

/*Defined Semaphores*/

/* Limits the 100 of students allowed to enter the waiting room at one time.*/
sem_t student_semaphore;
/* Used to allow the room keeper to sleep until a student arrives.*/
sem_t room_keeper_semaphore[CLASSRO0M];
/* Capacity of each classroom*/
sem_t classroom_semaphore[CLASSRO0M];
/* it holds, Is classroom cleaning or full*/
sem_t status_classroom_semaphore[CLASSRO0M];
/* For monitorize correctly*/
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;


void *student(void *id) {
    int max = 0, current = 0, rkpsleep = 0, index = 0, cls_id = 0; // for variables choose the classroom
    int classroom[CLASSRO0M], idle_classes[CLASSRO0M];
    sleep(rand() % 5); //a random time has been created for the student to arrive

    int std_id = *(int *) id;
    pthread_mutex_lock(&print_lock);
    printf(ANSI_COLOR_GREEN "Student %d came to the library waiting room.\n" ANSI_COLOR_RESET, std_id);
    pthread_mutex_unlock(&print_lock);

    int i = 0;
    for (i = 0; i < CLASSRO0M; i++) {
        sem_getvalue(status_classroom_semaphore + i, &rkpsleep);
        sem_getvalue(classroom_semaphore + i, &current);
        classroom[i] = current;
        if (current >= max && current < 4 && !rkpsleep == 0) {
            max = current;
        }
    }

    for (i = 0; i < CLASSRO0M; i++) {
        if (max == classroom[i]) {
            idle_classes[index++] = i;
        }
    }

    /*if the room is idle and in cleaning condition*/
    int kpsleep = 0;
    cls_id = idle_classes[rand() % index];
    sem_getvalue(room_keeper_semaphore + cls_id, &kpsleep);
    sem_post(room_keeper_semaphore + cls_id);
    sem_post(classroom_semaphore + cls_id);

    pthread_mutex_lock(&print_lock);
    printf(ANSI_COLOR_RED "%d th Student entered %dth classroom \n" ANSI_COLOR_RESET, std_id, cls_id);
    monitorize_current_event();
    pthread_mutex_unlock(&print_lock);

}

void *room_keeper(void *rkpid) {
    int rk_id = *(int *) rkpid;

    while (1) { // the loop is kill when classroom is full
        pthread_mutex_lock(&print_lock);
        printf("The %d room is cleaning classroom\n", rk_id - 1);
        pthread_mutex_unlock(&print_lock);
        // room keeper cleaning the classroom at the beginning
        sem_wait(room_keeper_semaphore + rk_id);

        int std_count = 0;
        while (std_count < WORKTABLE) {
            sem_getvalue(classroom_semaphore + rk_id, &std_count);
            if (std_count > 1 && std_count < WORKTABLE) {
                pthread_mutex_lock(&print_lock);    // announce classroom status
                printf(ANSI_COLOR_RED "The last %d students, let's get up!\n" ANSI_COLOR_RESET,
                       (WORKTABLE - std_count));
                pthread_mutex_unlock(&print_lock);
            }
            usleep(rand() % 3000 + 3000);
        }

        if (std_count >= 4) { // if classroom is full
            sem_wait(status_classroom_semaphore + rk_id);
            usleep(rand() % 3000 + 3000);

            int i;
            for (i = 0; i < WORKTABLE; i++) // free to students
                sem_wait(&student_semaphore);

            for (i = 0; i < WORKTABLE; i++) // empty to class
                sem_wait(classroom_semaphore + rk_id);

            pthread_mutex_lock(&print_lock); // if class empty print it
            printf(ANSI_COLOR_RED "Classroom %d is empty now!!!"ANSI_COLOR_RESET, rk_id);
            monitorize_current_event();
            pthread_mutex_unlock(&print_lock);

            int std_total = 0;
            sem_getvalue(&student_semaphore, &std_total); // number of remaining students
            pthread_mutex_lock(&print_lock);
            printf("%d students wait in library\n", std_total);
            pthread_mutex_unlock(&print_lock);
            sem_post(status_classroom_semaphore + rk_id);

            for (i = 0; i < WORKTABLE; i++)
                sem_wait(room_keeper_semaphore + rk_id);
        }
    }
}

void monitorize_current_event() {
    int i = 0, std_count = 0;
    printf(ANSI_COLOR_GREEN"\n");
    printf("+-------------------+\n");
    printf("|--- DEU Library ---|\n");
    printf("+-------------------+\n");
    for (i = 0; i < CLASSRO0M; ++i) {
        sem_getvalue(classroom_semaphore + i, &std_count);
        printf("Clasroom %d  : %d\n", i, std_count);
    }
    printf("\n"ANSI_COLOR_RESET);
}

int main() {
    pthread_t classroom_id[CLASSRO0M];
    pthread_t student_id[MAX_STUDENT];
    int lib_seats[MAX_STUDENT];

    int i;
    for (i = 0; i < MAX_STUDENT; i++)
        lib_seats[i] = i;

    sem_init(&student_semaphore, 0, MAX_STUDENT);

    // Initialize 'room_keeper_semaphore'
    for (i = 0; i < CLASSRO0M; i++)
        sem_init(room_keeper_semaphore + i, 0, 0);
    // Initialize 'classroom_semaphore'
    for (i = 0; i < CLASSRO0M; i++)
        sem_init(classroom_semaphore + i, 0, 0);
    // Initialize 'status_classroom_semaphore'
    for (i = 0; i < CLASSRO0M; i++)
        sem_init(status_classroom_semaphore + i, 0, 1);
    // Create classroom and students
    for (i = 0; i < CLASSRO0M; i++)
        pthread_create(classroom_id + i, NULL, &room_keeper, (void *) &lib_seats[i]);

    for (i = 0; i < MAX_STUDENT; i++)
        pthread_create(student_id + i, NULL, &student, (void *) &lib_seats[i]);

    // join all threads
    for (i = 0; i < MAX_STUDENT; i++)
        pthread_join(student_id[i], NULL);

    for (i = 0; i < CLASSRO0M; i++)
        pthread_join(classroom_id[i], NULL);

    return 0;
}
