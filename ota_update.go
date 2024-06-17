package main

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"log"
	"net"
	"os"
)

const chunkSize = 1024

func launchOtaUpdate(mcuIP, mcuPort string, certData, firmwareData []byte) error {
    certPool := x509.NewCertPool()
    if !certPool.AppendCertsFromPEM(certData) {
        return fmt.Errorf("failed to append certs from PEM")
    }

    tlsConfig := &tls.Config{
        RootCAs: certPool,
	/* InsecureSkipVerify should be used for testing only */
	InsecureSkipVerify: true,
    }

    conn, err := tls.Dial("tcp", net.JoinHostPort(mcuIP, mcuPort), tlsConfig)

    if err != nil {
        return fmt.Errorf("unable to connect: %w", err)
    }
    defer conn.Close()

    for offset := 0; offset < len(firmwareData); offset += chunkSize {
        end := offset + chunkSize
        if end > len(firmwareData) {
            end = len(firmwareData)
        }

        _, err := conn.Write(firmwareData[offset:end])
        if err != nil {
             return fmt.Errorf("error sending data: %w", err)
	}
    }

    fmt.Println("Firmware update sent successfully")
    return nil
}

func main() {
    if len(os.Args) < 5 {
        log.Fatal("Usage: go run ota_update.go <mcuIP> <mcuPort> <certFile> <firmwareFile>")
    }

    mcuIP := os.Args[1]
    mcuPort := os.Args[2]
    certFilePath := os.Args[3]
    firmwareFilePath := os.Args[4]

    /* Cert File */
    certData, err := os.ReadFile(certFilePath)
    if err != nil {
        log.Fatalf("Unable to read certificate file: %v", err)
    }

    /* Firmware file */
    firmwareData, err := os.ReadFile(firmwareFilePath)
    if err != nil {
        log.Fatalf("Unable to read firmware file: %v", err)
    }

    err = launchOtaUpdate(mcuIP, mcuPort, certData, firmwareData)
    if err != nil {
        log.Fatalf("OTA update failed: %v", err)
    }
}

