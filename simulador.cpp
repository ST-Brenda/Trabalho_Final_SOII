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






// Felipe adicionar conf. simulaçao





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







// Felipe adicionar função central








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
