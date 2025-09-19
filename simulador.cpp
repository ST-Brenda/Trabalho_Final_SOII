// ======================================================================================
//   Trabalho Final de Sistemas Operacionais II
//   Simulador de Memória Virtual com Paginação por Demanda
//
//   Dupla: Brenda Slongo Taca e Felipe Borges da Silva
//
//   Descrição breve: Este programa simula o funcionamento da memória virtual
//   utilizando diferentes algoritmos de substituição de página (FIFO, Optimal e Clock).
//   Ele permite configurar o número de frames, a sequência de acessos e também
//   comparar as políticas de alocação Global e Local.
// ======================================================================================


// ===========================
// Bibliotecas
// ===========================

#include <iostream>  // Para entrada e saída no console (cout, cin)
#include <vector>    // Para usar vetores dinâmicos (ex: memoria_fisica)
#include <queue>     // Estrutura de fila, para o algoritmo FIFO
#include <string>    // Para manipular as mensagens do log de eventos
#include <algorithm> // Para usar funções como std::find
#include <limits>    // Para limpar o buffer do cin
#include <map>       // Estrutura de mapa, que escolhemos para a Tabela de Páginas

// ===========================
// Constantes tipos Globais
// ===========================

// Definimos as constantes para as cores, pro código ficar mais fácil de entender e
// e ler com as mensagens coloridas no terminal.
const std::string RESETAR_COR = "\033[0m";
const std::string VERMELHO_NEGRITO = "\033[1;31m";
const std::string VERDE_NEGRITO = "\033[1;32m";
const std::string AMARELO_NEGRITO = "\033[1;33m";
const std::string AZUL_ESCURO = "\033[4;34m";

// Usamos um enum para os algoritmos pra ficar mais legível 
// sem uso de "números mágicos"
enum AlgoritmoSelecao {
    FIFO = 1,
    OPTIMAL = 2,
    CLOCK = 3
};

// ===========================
// Variáveis Globais
// ===========================

// Variáveis que precisam ser acessadas por várias funções durante a simulação.

int total_frames_fisicos;     // Total de frames na memória física simulada
AlgoritmoSelecao algoritmo;   // Qual algoritmo foi escolhido pelo usuário
char modo_execucao;           // 'P' para passo a passo, 'D' para direto
int qtd_paginas;              // Tamanho da sequência de acessos
bool modo_global = true;      // Política de alocação (Global ou Local)
std::vector<int> frames_alocados_processo; // Guarda os frames que o processo pode usar

// Struct pra representar entrada na Tabela de Páginas.
struct Pagina {
    int frame = -1;           // Em qual frame da memória física está a página (-1 se não estiver)
    bool presente = false;    // Bit de Presença/Ausência (P/A)
    bool referencia = false;  // Bit de Referência (R), pra usar com Clock
};

// A Tabela de Páginas. Usamos map porque é mais flexível.
// A chave é o número da página virtual, e o valor é a struct com os dados dela.
std::map<int, Pagina> tabela_paginas;

// Memória Física, vetor onde cada índice é um frame.
// O número da página que está ocupando aquele frame é guardado.
std::vector<int> memoria_fisica;


// =======================================================
// Declaração das Funções 
// =======================================================
// Mostra o menu inicial e coleta os dados informados pelo usuário para rodar a simulação
void configuracao_da_simulacao(std::vector<int>& sequencia);
// Mostra na tela o estado atual completo da simulação, seguindo o layout do nosso mockup :)
void print_estado_simulacao(const std::string& log_evento, const std::vector<int>& sequencia, int passo_atual, int ponteiro_clock = -1);
// Função principal, que percorre a sequência de acessos e usa a lógica de paginação e substituição.
int simular(const std::vector<int>& sequencia);


// =================================================================================
// Toda a parte da configuração da simulaçao
// =================================================================================
void configuracao_da_simulacao(std::vector<int>& sequencia) {
    // Imprime o cabeçalho do nosso simulador
    std::cout << "========================================================\n";
    std::cout << "| SIMULADOR DE MEMORIA VIRTUAL - PAGINACAO POR DEMANDA |\n";
    std::cout << "|             CONFIGURACAO DA SIMULACAO                |\n";
    std::cout << "========================================================\n\n";

    // Pega as entradas do usuário uma a uma, com validação para as escolhas.
    std::cout << "1. Informe o numero TOTAL de frames na memoria fisica: ";
    std::cin >> total_frames_fisicos;

    std::cout << "\n2. Informe o numero de acessos as paginas: ";
    std::cin >> qtd_paginas;

    sequencia.resize(qtd_paginas);
    std::cout << "   Digite a sequencia de acessos (separados por espaco):\n   ";
    for (int i = 0; i < qtd_paginas; i++) {
        std::cin >> sequencia[i];
    }

    int escolha_algoritmo = 0;
    while (escolha_algoritmo < 1 || escolha_algoritmo > 3) {
        std::cout << "\n3. Escolha o algoritmo de substituicao de pagina:\n";
        std::cout << "    [1] FIFO (First-In, First-Out)\n";
        std::cout << "    [2] Optimal (Otimo)\n";
        std::cout << "    [3] Clock (Segunda-Chance)\n";
        std::cout << "Sua escolha: ";
        std::cin >> escolha_algoritmo;
    }
    algoritmo = static_cast<AlgoritmoSelecao>(escolha_algoritmo);

    modo_execucao = ' ';
    while (toupper(modo_execucao) != 'P' && toupper(modo_execucao) != 'D') {
        std::cout << "\n4. Escolha o modo de execucao:\n";
        std::cout << "    [P] Passo a passo\n";
        std::cout << "    [D] Direto\n";
        std::cout << "Sua escolha: ";
        std::cin >> modo_execucao;
    }
    
    int escolha_alocacao = 0;
    while (escolha_alocacao < 1 || escolha_alocacao > 2) {
        std::cout << "\n5. Escolha a politica de alocacao de frames:\n";
        std::cout << "    [1] Global (processo pode usar todos os frames)\n";
        std::cout << "    [2] Local (processo tem uma particao fixa de frames)\n";
        std::cout << "Sua escolha: ";
        std::cin >> escolha_alocacao;
    }

    // Lógica pra definir os frames que o processo pode usar
    frames_alocados_processo.clear();
    if (escolha_alocacao == 1) { // Modo Global
        modo_global = true;
        // Se for global, o processo pode usar todos os frames da memória
        for (int i = 0; i < total_frames_fisicos; ++i) {
            frames_alocados_processo.push_back(i);
        }
    } else { // Modo Local
        modo_global = false;
        int num_frames_proc = 0;
        std::cout << "   -> Modo Local: Quantos frames (de um total de " << total_frames_fisicos << ") este processo pode usar? ";
        std::cin >> num_frames_proc;
        while(num_frames_proc > total_frames_fisicos || num_frames_proc <= 0){
             std::cout << "   Valor invalido. Tente novamente: ";
             std::cin >> num_frames_proc;
        }
        // Pra simplificar, definimos que o processo usa os 'N' primeiros frames. 
        for (int i = 0; i < num_frames_proc; ++i) {
            frames_alocados_processo.push_back(i); 
        }
    }

    // Pausa pro usuário ver as configurações e iniciar a simulação.
    std::cout << "\n==================================================\n";
    std::cout << "Pressione Enter para iniciar a simulacao...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

// =================================================================================
// Mostra o estado da simulação
// =================================================================================
void print_estado_simulacao(const std::string& log_evento, const std::vector<int>& sequencia, int passo_atual, int ponteiro_clock) {
    // Comentamos o comando de limpar a tela
    // Caso queira usar, é só descomentar:
    // #ifdef _WIN32
    //     system("cls");
    // #else
    //     system("clear");
    // #endif

    // Mostra a sequência de acesso, destacando a página que está sendo processada
    std::cout << AZUL_ESCURO << "\n================ SIMULADOR DE MEMORIA VIRTUAL =================\n" << RESETAR_COR;
    std::cout << "Sequencia: ";
    for(int i=0; i < sequencia.size(); ++i) {
        if(i == passo_atual) std::cout << AMARELO_NEGRITO << "[" << sequencia[i] << "] " << RESETAR_COR;
        else std::cout << sequencia[i] << " ";
    }
    std::cout << "\nPolitica de Alocacao: " << (modo_global ? "Global" : "Local") << " | Frames do Processo: " << frames_alocados_processo.size() << "\n\n";

    // Mostra o estado da Memória Física, com qual página ocupa cada frame
    // Também indica o ponteiro do Clock e, no modo Local, quais frames são de outros processos
    std::cout << AMARELO_NEGRITO << "MEMORIA FISICA (Frames)\n" << RESETAR_COR;
    for(int f=0; f < total_frames_fisicos; ++f) {
        bool pertence_processo = (std::find(frames_alocados_processo.begin(), frames_alocados_processo.end(), f) != frames_alocados_processo.end());
        
        std::cout << "F" << f << ": ";
        if(memoria_fisica[f] != -1) {
            std::cout << "Pagina " << memoria_fisica[f];
        } else {
            std::cout << "Livre";
        }
        if(!pertence_processo && !modo_global) {
             std::cout << " (Outro Processo)";
        }
        if(algoritmo == CLOCK && f == ponteiro_clock) {
            std::cout << AMARELO_NEGRITO << "  <-- Ponteiro" << RESETAR_COR;
        }
        std::cout << "\n";
    }
    std::cout << "\n";

    // Mostra a tabela de páginas, apenas as páginas que estão na memória
    std::cout << AMARELO_NEGRITO << "TABELA DE PAGINAS (P/A = Presente/Ausente, R = Referencia)\n" << RESETAR_COR;
    std::cout << "Pag | Frame | P/A | R\n";
    for(auto const& [num_pag, dados_pag] : tabela_paginas){
        if(dados_pag.presente) {
            std::cout << num_pag << "   | " << dados_pag.frame << "     | " 
                      << VERDE_NEGRITO << dados_pag.presente << RESETAR_COR << "   | " << dados_pag.referencia << "\n";
        }
    }
    std::cout << "\n";
    
    // Exibeo log de eventos, que descreve o que aconteceu na simulação
    std::cout << AMARELO_NEGRITO << "LOG DE EVENTOS\n" << RESETAR_COR << log_evento << "\n\n";
}


// =================================================================================
// Função central, simula a paginação e a substituição
// =================================================================================
int simular(const std::vector<int>& sequencia) {
    int page_faults = 0;
    std::string log;
    std::queue<int> fila_fifo; // Fila pro algoritmo FIFO
    int ponteiro_clock = 0;   // Ponteiro pro algoritmo Clock

    // O loop principal percorre cada página na sequência de acessos
    for (int i = 0; i < sequencia.size(); i++) {
        int pagina_atual = sequencia[i];
        
        // Se for a primeira vez que essa página está sendo vista, é criada uma entrada pra ela na tabela
        if(tabela_paginas.find(pagina_atual) == tabela_paginas.end()){
            tabela_paginas[pagina_atual] = Pagina();
        }
        
        log = "Acesso a pagina " + std::to_string(pagina_atual) + " -> ";

        // Verifica se a página está na memória (PAGE HIT ou PAGE FAULT)
        if (tabela_paginas[pagina_atual].presente) { // PAGE HIT
            log += VERDE_NEGRITO + "PAGE HIT!" + RESETAR_COR;
            // Se for Clock, marcamos o bit de referência como 1.
            if (algoritmo == CLOCK) {
                tabela_paginas[pagina_atual].referencia = true;
            }
        } else { // PAGE FAULT
            log += VERMELHO_NEGRITO + "PAGE FAULT!" + RESETAR_COR + "\n";
            page_faults++;

            // Primeiro passo do Page Fault: procura um frame livre na partição do processo
            int frame_livre = -1;
            for (int frame_idx : frames_alocados_processo) {
                if (memoria_fisica[frame_idx] == -1) {
                    frame_livre = frame_idx;
                    break;
                }
            }

            // Se achou, usa ele
            if (frame_livre != -1) {
                memoria_fisica[frame_livre] = pagina_atual;
                tabela_paginas[pagina_atual] = {frame_livre, true, true};
                if(algoritmo == FIFO) fila_fifo.push(pagina_atual);
                log += "> Pagina " + std::to_string(pagina_atual) + " carregada no frame livre " + std::to_string(frame_livre);
            } else { // Se não tiver frame livre, precisa substituir uma página.
                log += "> Memoria cheia na particao do processo. Iniciando substituicao...\n";
                int frame_vitima = -1;
                int pagina_vitima = -1;

                // Aqui escolhe o algoritmo de substituição que o usuário selecionou
                switch(algoritmo) {
                    case FIFO: {
                        pagina_vitima = fila_fifo.front();
                        fila_fifo.pop();
                        frame_vitima = tabela_paginas[pagina_vitima].frame;
                        log += "> Algoritmo FIFO: Pagina " + std::to_string(pagina_vitima) + " (a mais antiga) foi escolhida.";
                        break;
                    }
                    case OPTIMAL: {
                        int maior_distancia = -1;
                        // Procura no futuro qual página da memória vai demorar mais pra ser usada
                        for (int frame_idx : frames_alocados_processo) {
                            int pagina_candidata = memoria_fisica[frame_idx];
                            int distancia_futura = std::numeric_limits<int>::max(); // O algoritmo assume que a página nunca mais vai ser usada
                            for (int j = i + 1; j < sequencia.size(); j++) {
                                if (sequencia[j] == pagina_candidata) {
                                    distancia_futura = j;
                                    break;
                                }
                            }
                            if (distancia_futura > maior_distancia) {
                                maior_distancia = distancia_futura;
                                frame_vitima = frame_idx;
                            }
                        }
                        pagina_vitima = memoria_fisica[frame_vitima];
                        log += "> Algoritmo Optimal: Pagina " + std::to_string(pagina_vitima) + " (uso mais distante no futuro) foi escolhida.";
                        break;
                    }
                    case CLOCK: {
                        // Loop do Clock: gira o ponteiro até achar uma página com bit de referência 0 (R=0)
                        while(frame_vitima == -1){
                            // A verificação é a correção para o modo Local
                            // Aqui garante que o ponteiro só vai considerar para substituição um frame que realmente pertence ao processo
                            bool frame_pertence_ao_processo = (std::find(frames_alocados_processo.begin(), frames_alocados_processo.end(), ponteiro_clock) != frames_alocados_processo.end());

                            if (frame_pertence_ao_processo) {
                                int pagina_no_ponteiro = memoria_fisica[ponteiro_clock];
                                if(!tabela_paginas[pagina_no_ponteiro].referencia){ // Achou a vítima (R=0)
                                    frame_vitima = ponteiro_clock;
                                    pagina_vitima = pagina_no_ponteiro;
                                } else { // Dá uma segunda chance setando o bit de referência (R=1 -> R=0)
                                    tabela_paginas[pagina_no_ponteiro].referencia = false;
                                }
                            }
                            // Avança o ponteiro pra próxima posição.
                            ponteiro_clock = (ponteiro_clock + 1) % total_frames_fisicos;
                        }
                        log += "> Algoritmo Clock: Pagina " + std::to_string(pagina_vitima) + " (com bit R=0) foi escolhida no frame " + std::to_string(frame_vitima);
                        break;
                    }
                }

                // Substitui a página.
                tabela_paginas[pagina_vitima].presente = false;
                tabela_paginas[pagina_vitima].frame = -1;
                
                memoria_fisica[frame_vitima] = pagina_atual;
                tabela_paginas[pagina_atual] = {frame_vitima, true, true};
                if(algoritmo == FIFO) fila_fifo.push(pagina_atual);
            }
        }
        
        // No fim de cada passo, exibe o estado atual e pausa se for o modo passo a passo.
        print_estado_simulacao(log, sequencia, i, (algoritmo == CLOCK ? ponteiro_clock : -1));
        if (toupper(modo_execucao) == 'P') {
            std::cout << "Pressione Enter para o proximo passo...";
            std::cin.get();
        }
    }
    return page_faults; // Retorna o total de page faults
}

// =================================================================================
// MAIN
// =================================================================================
int main() {
    std::vector<int> sequencia;

    // Chama a função de configuração.
    configuracao_da_simulacao(sequencia);

    // Inicializa as estruturas de dados.
    memoria_fisica.assign(total_frames_fisicos, -1);
    
    // Roda a simulação.
    int page_faults = simular(sequencia);

    // Imprime o resumo final com as métricas.
    std::cout << "\n" << AZUL_ESCURO << "================== SIMULACAO FINALIZADA ==================\n" << RESETAR_COR;
    std::cout << "Total de acessos: " << qtd_paginas << std::endl;
    std::cout << "Total de page faults: " << page_faults << std::endl;
    if (qtd_paginas > 0) {
        double taxa = 100.0 * static_cast<double>(page_faults) / qtd_paginas;
        std::cout.precision(2); // Para formatar a saída com 2 casas decimais
        std::cout << "Taxa de page faults: " << std::fixed << taxa << "%\n";
    }

    return 0; 
}
