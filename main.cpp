#include <ctime>
#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <iterator>

#define MAX_PROCESSES 10

int current_process_index = -1;
int process_count = 0;

// Função de sleep (em ms)
// [Ref] stackoverflow.com/a/28827188/4824627
void sleep_ms(int milliseconds) {
  struct timespec ts;
  ts.tv_sec = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

enum IOTypes {
  IO_DISCO = 0,
  IO_FITA,
  IO_IMPRESSORA
};

enum Priorities {
  PRIORITY_HIGH = 0,
  PRIORITY_LOW,
  PRIORITY_IO
};

enum Status {
  STATUS_NEW = 0,   // Foi criado, ainda não está na fila
  STATUS_READY,     // Esperando ser executado
  STATUS_RUNNING,   // Está sendo executado
  STATUS_WAITING,   // Entrou em IO / realizando IO
  STATUS_TERMINATED // Processo acabou
};

int getIODuration(int io) {
  // TODO: modificar os tempos
  switch(io) {
    case IO_DISCO:
      return 100;
    case IO_FITA:
      return 101;
    case IO_IMPRESSORA:
      return 102;
    default:
      return 0;
  }
}

typedef struct IO_Operation {
  int type;             // Identificador do IO
  int start_time;       // Momento de início do IO
} IO_Operation;

class Process {
public:
  int PID;                      // Identificador do processo
  int PPID = 1;                 // Identificador do processo pai (sempre = 1)
  int status = STATUS_NEW;      // Status do processo
  int priority = PRIORITY_HIGH; // Nível de prioridade do processo
  int total_time;               // Tempo total a ser executado
  int start_time;               // Momento em que o processo inicia
  int elapsed_time = 0;         // Tempo já executado
  IO_Operation* IOs;            // Array de momentos de I/O

public:
  // Caso só exista parâmetro de tempo total, assumimos que não há I/O,
  // e que o processo inicia imediatamente
  Process(int total_time):
    PID( generatePID() ),
    total_time(total_time),
    start_time(0) {}

  Process(int total_time, int start_time):
    PID( generatePID() ),
    total_time(total_time),
    start_time(start_time) {}

  Process(int total_time, IO_Operation* ios):
    PID( generatePID() ),
    total_time(total_time),
    start_time(0),
    IOs(ios) {}

  Process(int total_time, int start_time, IO_Operation* ios):
    PID( generatePID() ),
    total_time(total_time),
    start_time(start_time),
    IOs(ios) {}

private:
  // Gera PID novo, exclusivo para o processo
  int generatePID() {
    // Se o número de processos ainda não ultrapassou o limite
    if(process_count < MAX_PROCESSES) {
      // Aumenta o número de processos
      process_count++;
      // Retorna o valor
      return process_count;
    }
    return -1;
  }
};

std::ostream& operator<<(std::ostream& o, const IO_Operation& op) {
  return o << "IO[" << op.type << ";" << op.start_time << "]";
}
std::ostream& operator<<(std::ostream& o, const IO_Operation* arr) {
  if(!arr)
    return o << "(empty)";

  for(int i = 0; i < 50; ++i) {
    if(arr[i].type == -1 || arr[i].start_time == -1)
      break;

    o << arr[i];
  }

  return o;
}
// Operador << para impressão de ponteiros de Process
std::ostream& operator <<(std::ostream& o, const Process* p) {
  return o << "[*Process](PID=" << p->PID << ")"
           << "(elapsed=" << p->elapsed_time << ")"
           << "(total=" << p->total_time << ")"
           << "(IOs=" << p->IOs << ")";
}

// Operador << para impressão de referências de Process
std::ostream& operator <<(std::ostream& o, const Process& p) {
  return o << "[Process](PID=" << p.PID << ")"
           << "(elapsed=" << p.elapsed_time << ")"
           << "(total=" << p.total_time << ")"
           << "(IOs=" << p.IOs << ")";
}

IO_Operation createIO(int type, int start_time) {
  IO_Operation ret;
  ret.type = type;
  ret.start_time = start_time;
  return ret;
}

// Quando são colocados dentro da função `initializeProcesses()`, essas
// variáveis são descartadas e removidas da memória; assim, o Process fica com
// lixo no lugar :(
IO_Operation IO_Limiter = createIO(-1,-1);
IO_Operation a[2] = { createIO(IO_IMPRESSORA,2), IO_Limiter };
IO_Operation b[3] = { createIO(IO_IMPRESSORA,4), createIO(IO_FITA,7), IO_Limiter };
IO_Operation c[4] = { createIO(IO_DISCO,1), createIO(IO_FITA,2), createIO(IO_IMPRESSORA,6),IO_Limiter };

// Inicializa processos
void initializeProcesses(Process** all_processes) {
  for(int i = 0; i < MAX_PROCESSES; ++i) {
    int t = i % 4;
    switch(t) {
      case 0: {
        all_processes[i] = new Process(5, (std::rand() % 10)+2, a);
        break;
      } case 1: {
        all_processes[i] = new Process(5, (std::rand() % 10)+2, b);
        break;
      } case 2: {
        all_processes[i] = new Process(5, (std::rand() % 10)+2, c);
        break;
      } case 3: {
        all_processes[i] = new Process(5, (std::rand() % 10)+2);
        break;
      }
    }
    std::cout << "Processo (PID " << all_processes[i]->PID << ") criado: "
              << all_processes[i] << std::endl;
  }
}

int hasUnfinishedProcess(Process** all_processes) {
  // Confere se algum processo não possui status de TERMINATED
  for(int i = 0; i < MAX_PROCESSES; ++i)
    if(all_processes[i]->status != STATUS_TERMINATED)
      return true;
  // Senão, se todos já possuem status TERMINATED
  return false;
}

void checkForNewProcess(Process** all_processes, int cycle_count) {
  for(int i = 0; i < MAX_PROCESSES; ++i) {
    if(all_processes[i]->start_time == cycle_count) {
      all_processes[i]->status = STATUS_READY;
      std::cout << "\tProcesso " << all_processes[i]->PID << " está pronto para ser executado!" << std::endl;
    }
  }
}
void checkForFinishedIO(int cycle_count) {
  // TODO: essa função
  (void)cycle_count; // (evita warnings de unused)
  std::cout << "\tConferindo por processos que terminaram IO..." << std::endl;
  std::cout << "\tNenhum processo terminou IO!" << std::endl;
}
void checkForPreemption() {
  // TODO: essa função
}
int tryRunNewProcess(Process** all_processes) {
  std::cout << "\tNão há processos executando. Procurando um processo com estado "
            << "'READY' para ser executado." << std::endl;
  for(int i = 0; i < MAX_PROCESSES; ++i) {
    if(all_processes[i]->status == STATUS_READY) {
      std::cout << "\tEncontrado! Executando processo " << all_processes[i]->PID << std::endl;
      all_processes[i]->status = STATUS_RUNNING;
      return i;
    }
  }
  std::cout << "\tNenhum processo encontrado!" << std::endl;
  return -1;
}

int main() {
  // Determina seed para a função de random
  srand(time(NULL));

  // Cria lista de todos os processos
  Process** all_processes;
  all_processes = (Process**)calloc(sizeof(Process*),MAX_PROCESSES);
  if(all_processes == NULL) {
    // Erro criando lista de processos
    std::cout << "calloc falhou!" << std::endl;
    exit(1);
  }
  // Inicializa os processos (hardcoded)
  initializeProcesses(all_processes);

  sleep_ms(500);
  std::cout << std::endl;
  std::cout << "Todos os processos criados! Iniciando processador..." << std::endl;
  std::cout << std::endl;
  sleep_ms(1500);

  // Contagem de ciclos do processador
  int global_cycle_count = 0;

  while(true) {
    std::cout << "Ciclo: " << global_cycle_count << std::endl;

    // TODO: código mais temporário que qualquer outra coisa
    // talvez mudar pra uma função separada seja melhor?
    if(current_process_index >= 0) {
      // TODO: conferir se está em IO etc
      Process* current_process = all_processes[current_process_index];
      current_process->elapsed_time++;
      std::cout << "\tExecutando processo " << current_process->PID << std::endl;
      int diff = current_process->total_time - current_process->elapsed_time;
      std::cout << "\tFalta " << (diff) << " u.t." << std::endl;
      if(diff == 0) {
        std::cout << "\tProcesso terminado!" << std::endl;
        current_process->status = STATUS_TERMINATED;
        current_process_index = -1;
      }
    } else {
      // Confere se ainda existe algum processo inacabado
      if(!hasUnfinishedProcess(all_processes))
        // Se não existe, termina o loop
        break;
    }

    sleep_ms(500);

    // Confere se existe algum processo que inicia nesse ciclo
    // (e o coloca na fila apropriada)
    checkForNewProcess(all_processes, global_cycle_count);

    sleep_ms(500);

    // Confere se algum processo acabou de terminar operação de I/O
    // (e o coloca na fila apropriada)
    checkForFinishedIO(global_cycle_count);

    sleep_ms(500);

    // Confere se o processo sendo executado deve sofrer preempção
    checkForPreemption();

    sleep_ms(500);

    // (TODO: conferir essa ordem de prioridades)
    if(current_process_index < 0)
      current_process_index = tryRunNewProcess(all_processes);

    sleep_ms(500);

    // Incrementa o ciclo atual
    global_cycle_count++;

    sleep_ms(1500);
  }

  std::cout << "Programa terminado!" << std::endl;
  return 0;
}
