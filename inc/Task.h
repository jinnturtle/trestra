struct Task {
    time_t creation_time; //date and time when the task was created
    time_t status_time; //when was the status altered
    char name[80];
    unsigned id;
    unsigned parent_id;
    unsigned orig_estimate; //original minutes estimated to complete the task
    unsigned estimate; //minutes estimated to complete the task
    unsigned fact; //how much time is already spent on the task
    int status; //task status (e.g. 0 - NIL, 1 - BACKLOG, 2 - NEXT, etc)
};
