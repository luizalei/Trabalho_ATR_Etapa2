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
    BOOL pausado = FALSE;

    printf("Thread de Visualizacao de Sinalizacao iniciada\n");

    while (1) {
        DWORD waitResult = WaitForSingleObject(hEventMsgDiscoDisponivel, 100);
        /*if (waitResult == WAIT_OBJECT_0 && !pausado) {
            WaitForSingleObject(hMutexArquivoDisco, INFINITE);*/

           

    //        // Processa cada mensagem
    //        int mensagens_processadas = 0;
    //        for (long i = 0; i < tamanho; i += MAX_MSG_LENGTH) {
    //            char mensagem[MAX_MSG_LENGTH + 1] = { 0 };
    //            memcpy(mensagem, buffer + i, MAX_MSG_LENGTH);

    //            // Processamento da mensagem (mantido igual ao seu código original)
    //            char nseq[8], tipo[3], diag[2], remota[4], id[9], estado[2], timestamp[13];
    //            int parsed = sscanf_s(mensagem, "%7[^;];%2[^;];%1[^;];%3[^;];%8[^;];%1[^;];%12s",
    //                nseq, (unsigned)sizeof(nseq),
    //                tipo, (unsigned)sizeof(tipo),
    //                diag, (unsigned)sizeof(diag),
    //                remota, (unsigned)sizeof(remota),
    //                id, (unsigned)sizeof(id),
    //                estado, (unsigned)sizeof(estado),
    //                timestamp, (unsigned)sizeof(timestamp));

    //            if (parsed == 7) {
    //                int estado_int = estado[0] - '0';
    //                if (estado_int >= 0 && estado_int < 20) {
    //                    const char* estadoTexto = estados_texto[estado_int];
    //                    printf("%s NSEQ: %s REMOTA: %s SENSOR: %s ESTADO: %s\n",
    //                        timestamp, nseq, remota, id, estadoTexto);
    //                    mensagens_processadas++;
    //                }
    //            }
    //        }

    //        printf("[INFO] Processadas %d mensagens\n", mensagens_processadas);

    //        // Limpa o buffer após processar
    //        free(buffer);

    //        // Limpa o arquivo após processamento
    //        freopen_s(&arquivo, ARQUIVO_DISCO, "w+b", arquivo);
    //        fclose(arquivo);
    //        ReleaseMutex(hMutexArquivoDisco);
    //    }

    //    // Controle de pausa e encerramento (mantido igual)
    //    DWORD result = WaitForMultipleObjects(2, eventos, FALSE, 0);
    //    /* ... resto do código igual ... */
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

    CloseHandle(hThread);
    CloseHandle(evVISUFERROVIA_PauseResume);
    CloseHandle(evVISUFERROVIA_Exit);
    CloseHandle(evVISUFERROVIATemporização);
    CloseHandle(evEncerraThreads);

    return 0;
}



