#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <io.h>

#define ARQUIVO_DISCO "arquivo_sinalizacao.dat"
#define MAX_MSG_LENGTH 41
#define MAX_MENSAGENS_DISCO 200

HANDLE evVISUFERROVIA_PauseResume;
HANDLE evVISUFERROVIA_Exit;
HANDLE evVISUFERROVIATemporização;
HANDLE evEncerraThreads;
HANDLE hEventMsgDiscoDisponivel;
HANDLE hEventEspacoDiscoDisponivel;
HANDLE hMutexArquivoDisco;
HANDLE hEventSemMsgNovas;

char* lpimage;// Apontador para imagem local

// 20 textos de estado
const char* estados_texto[20] = {
    "Desvio atuado",
    "Sinaleiro em PARE",
    "Sinaleiro em VIA LIVRE",
    "Ocorrencia na via",
    "Sensor inativo",
    "Veiculo detectado",
    "Barreira abaixada",
    "Barreira levantada",
    "Desvio nao confirmado",
    "Sensor com falha",
    "Via ocupada",
    "Via livre",
    "Alarme de intrusao",
    "Alimentacao normal",
    "Alimentacao interrompida",
    "Sinaleiro apagado",
    "Controle manual ativado",
    "Controle automatico ativo",
    "Velocidade excedida",
    "Falha de comunicacao"
};

DWORD WINAPI ThreadVisualizaSinalizacao(LPVOID) {
    HANDLE eventos[2] = { evVISUFERROVIA_PauseResume, evEncerraThreads };
    HANDLE hArquivoDiscoMapping;
    BOOL pausado = FALSE;

    printf("Thread de Visualizacao de Sinalizacao iniciada\n");

    // Abre o arquivo em modo leitura na mesma visão que a main.cpp
    hArquivoDiscoMapping = OpenFileMapping(FILE_MAP_READ, FALSE, L"MAPEAMENTO");

    //Checagem de erro para OpenFileMapping
    if (hArquivoDiscoMapping == NULL) {
        DWORD erro = GetLastError();

        // Converte o código de erro em uma mensagem legível
        LPVOID mensagemErro;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            erro,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&mensagemErro,
            0,
            NULL
        );

        printf("Erro ao abrir o mapeamento de arquivo. Codigo: %d - Mensagem: %s\n", erro, (char*)mensagemErro);
        LocalFree(mensagemErro);
    }
    else {
        printf("Mapeamento aberto com sucesso!\n");
    }

    // Mapeando a mesma visão do arquivo que a main.cpp
    lpimage = (char*)MapViewOfFile(hArquivoDiscoMapping, FILE_MAP_READ, 0, 0, MAX_MENSAGENS_DISCO);

    // Checagem de erro para MapViewOfFile
    if (lpimage == NULL) {
        DWORD erro = GetLastError();
        LPVOID mensagemErro = NULL;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            erro,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&mensagemErro,
            0,
            NULL
        );

        printf("Falha ao mapear a view do arquivo. Erro %d: %s\n",
            erro, (char*)mensagemErro);

        LocalFree(mensagemErro);

        return FALSE; // ou outro código de erro apropriado
    }

    while (1) {
        if (!pausado) {
            DWORD waitResult = WaitForSingleObject(hEventMsgDiscoDisponivel, INFINITE); //Espera que hajam mensagens escritas
            if (waitResult == WAIT_OBJECT_0 && !pausado) {

                WaitForSingleObject(hMutexArquivoDisco, INFINITE); //Garante acesso único ao arquivo 
                ResetEvent(hEventMsgDiscoDisponivel); // Avisa a main que recebeu a mensagem


                // Processa cada mensagem
                int mensagens_processadas = 0;
                long tamanho = strlen(lpimage); 

                for (long i = 0; i < tamanho; i += MAX_MSG_LENGTH) {
                    char mensagem[MAX_MSG_LENGTH + 1] = { 0 };
                    size_t copy_size = (tamanho - i) < MAX_MSG_LENGTH ? (tamanho - i) : MAX_MSG_LENGTH;
                    memcpy_s(mensagem, MAX_MSG_LENGTH, lpimage + i, copy_size);
                    mensagem[copy_size] = '\0'; // Garante terminação nula

                    // Processamento da mensagem 
                    char nseq[8] = { 0 }, tipo[3] = { 0 }, diag[2] = { 0 },
                        remota[4] = { 0 }, id[9] = { 0 }, estado[2] = { 0 },
                        timestamp[13] = { 0 };

                    int parsed = sscanf_s(mensagem, "%7[^;];%2[^;];%1[^;];%3[^;];%8[^;];%1[^;];%12s",
                        nseq, (unsigned)_countof(nseq),
                        tipo, (unsigned)_countof(tipo),
                        diag, (unsigned)_countof(diag),
                        remota, (unsigned)_countof(remota),
                        id, (unsigned)_countof(id),
                        estado, (unsigned)_countof(estado),
                        timestamp, (unsigned)_countof(timestamp));

                    if (parsed == 7) {
                        int estado_int = estado[0] - '0';
                        if (estado_int >= 0 && estado_int < 20) {
                            const char* estadoTexto = estados_texto[estado_int];
                            printf("%s NSEQ: %s REMOTA: %s SENSOR: %s ESTADO: %s\n",
                                timestamp, nseq, remota, id, estadoTexto);
                            mensagens_processadas++;
                        }
                    }
                }
                // Limpa o buffer após processar
                //memset(lpimage, 0, MAX_MENSAGENS_DISCO); // Limpa a memória mapeada

                SetEvent(hEventSemMsgNovas); // Sinaliza que novas mensagens foram processadas
                ReleaseMutex(hMutexArquivoDisco);
            }

            DWORD result = WaitForMultipleObjects(2, eventos, FALSE, 0);

        }

        // Verifica os dois eventos simultaneamente (sem bloquear)
        DWORD result = WaitForMultipleObjects(2, eventos, FALSE, 0);

        switch (result) {
        case WAIT_OBJECT_0:  // evVISUFERROVIA_PauseResume
            pausado = !pausado;
            if (pausado) {
                printf("[Sinalizacao] Thread pausada. Aguardando retomada...\n");
            }
            else {
                printf("[Sinalizacao] Retomando execucao.\n");
            }
            ResetEvent(evVISUFERROVIA_PauseResume);
            break;

        case WAIT_OBJECT_0 + 1:  // evVISUFERROVIA_Exit
            printf("[Sinalizacao] Evento de saida recebido. Encerrando thread.\n");
            return 0;

        default:
            break; // Nenhum evento sinalizado
        }

        // Se pausado, entra em espera bloqueante ate algo acontecer
        while (pausado) {
            DWORD r = WaitForMultipleObjects(2, eventos, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) { // Toggle pausa
                pausado = FALSE;
                printf("[Sinalizacao] Retomando execucao.\n");
                ResetEvent(evVISUFERROVIA_PauseResume);
                break;
            }
            else if (r == WAIT_OBJECT_0 + 1) { // Evento de saida
                printf("[Sinalizacao] Evento de saida recebido. Encerrando thread.\n");
                return 0;
            }
        }
    }

    return 0;

}

int main() {
    evVISUFERROVIA_PauseResume = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EV_VISUFERROVIA_PAUSE");
    evVISUFERROVIA_Exit = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EV_VISUFERROVIA_EXIT");
    evEncerraThreads = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EV_ENCERRA_THREADS");
    evVISUFERROVIATemporização = CreateEvent(NULL, FALSE, FALSE, L"EV_VISUFERROVIA_TEMPORIZACAO"); // evento que nunca será setado apenas para temporização
    hEventEspacoDiscoDisponivel = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EV_ESPACO_DISCO_DISPONIVEL");
    hMutexArquivoDisco = OpenMutex(MUTEX_ALL_ACCESS, FALSE, L"MUTEX_ARQUIVO_DISCO");
    hEventMsgDiscoDisponivel = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EV_MSG_DISCO_DISPONIVEL");
    hEventSemMsgNovas = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EV_SEM_MSG_NOVAS");

    if (hEventMsgDiscoDisponivel == NULL) {
        printf("[Erro] Falha ao abrir EV_MSG_DISCO_DISPONIVEL: %lu\n", GetLastError());
        return 1;
    }
    if (hMutexArquivoDisco == NULL) {
        printf("[Erro] Falha MUTEX_ARQUIVO_DISCO: %lu\n", GetLastError());
        return 1;
    }
    HANDLE hThread = CreateThread(NULL, 0, ThreadVisualizaSinalizacao, NULL, 0, NULL);
    if (hThread == NULL) {
        printf("Erro ao criar a thread.\n");
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);

    BOOL status;
    status = UnmapViewOfFile(lpimage);
    // Checagem de erro
    if (!status) {
        DWORD erro = GetLastError();
        LPVOID mensagemErro = NULL;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            erro,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&mensagemErro,
            0,
            NULL
        );

        printf("Falha ao desmapear a view do arquivo. Erro %d: %s\n",
            erro, (char*)mensagemErro);

        LocalFree(mensagemErro);
    }

    CloseHandle(hThread);
    CloseHandle(evVISUFERROVIA_PauseResume);
    CloseHandle(evVISUFERROVIA_Exit);
    CloseHandle(evVISUFERROVIATemporização);
    CloseHandle(evEncerraThreads);
    CloseHandle(hEventSemMsgNovas);

    return 0;
}



