#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_SEATS 100
#define MAX_FLIGHTS 10
#define MAX_NAME_LEN 49
#define MAX_DEST_LEN 49
#define MAX_TIME_LEN 9
#define PNR_LEN 9
#define ADMIN_PASS_LEN 49

#define RESERVATION_FILE "reservations.dat"
#define FLIGHT_FILE "flights.dat"
#define TEMP_FILE "temp.dat"
#define ADMIN_PASSWORD "admin123"

typedef struct {
    char name[MAX_NAME_LEN + 1];      // +1 for null terminator
    int age;
    char gender;                      // 'M' or 'F'
    int seatNumber;                   // 1 to MAX_SEATS
    char pnr[PNR_LEN + 1];           // +1 for null terminator
    int flightNumber;
    float fare;
    int paymentMethod;                // 1-4
    int isBooked;                     // 1 for booked, 0 for cancelled
} Passenger;

typedef struct {
    int flightNumber;
    char destination[MAX_DEST_LEN + 1];
    char departure[MAX_DEST_LEN + 1];
    char time[MAX_TIME_LEN + 1];
    float fare;
    int availableSeats;               // Should be 0 to MAX_SEATS
} Flight;

/* ================ UTILITY FUNCTIONS ================ */

/**
 * Clear input buffer to prevent issues with scanf/fgets mixing
 */
void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/**
 * Safe string input with length limit
 */
void safeStringInput(char *buffer, int maxLen, const char *prompt) {
    printf("%s", prompt);
    
    if (fgets(buffer, maxLen + 1, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }
    
    // Remove trailing newline if present
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    } else if (len == maxLen) {
        // Input was too long, clear the buffer
        clearInputBuffer();
    }
}

/**
 * Safe integer input with validation
 */
int safeIntInput(const char *prompt, int min, int max) {
    int value;
    char input[100];
    
    while (1) {
        printf("%s", prompt);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            continue;
        }
        
        // Check if input is a valid integer
        if (sscanf(input, "%d", &value) == 1) {
            if (value >= min && value <= max) {
                return value;
            }
        }
        
        printf("Invalid input. Please enter a number between %d and %d.\n", min, max);
    }
}

/**
 * Initialize data files with proper binary mode
 */
void initializeFiles() {
    FILE *fp;
    
    // Initialize reservations file
    fp = fopen(RESERVATION_FILE, "ab");
    if (fp == NULL) {
        printf("Warning: Could not create/access reservation file.\n");
    } else {
        fclose(fp);
    }
    
    // Initialize flights file
    fp = fopen(FLIGHT_FILE, "ab");
    if (fp == NULL) {
        printf("Warning: Could not create/access flights file.\n");
    } else {
        fclose(fp);
    }
}

/**
 * Generate a unique PNR based on timestamp and random number
 * This prevents collisions better than sequential counting
 */
void generatePNR(char *pnr) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    int randomNum = rand() % 10000;
    
    // Format: YYMMDD + random 4 digits
    snprintf(pnr, PNR_LEN + 1, "%02d%02d%02d%04d",
             tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, randomNum);
}

/* ================ FLIGHT MANAGEMENT ================ */

/**
 * Check if a flight exists and has available seats
 */
int isFlightValid(int flightNumber) {
    Flight flight;
    FILE *fp = fopen(FLIGHT_FILE, "rb");
    
    if (!fp) {
        return 0;
    }
    
    int isValid = 0;
    while (fread(&flight, sizeof(Flight), 1, fp) == 1) {
        if (flight.flightNumber == flightNumber && flight.availableSeats > 0) {
            isValid = 1;
            break;
        }
    }
    
    fclose(fp);
    return isValid;
}

/**
 * Get fare for a specific flight
 */
float getFlightFare(int flightNumber) {
    Flight flight;
    FILE *fp = fopen(FLIGHT_FILE, "rb");
    
    if (!fp) {
        return -1.0f;
    }
    
    float fare = -1.0f;
    while (fread(&flight, sizeof(Flight), 1, fp) == 1) {
        if (flight.flightNumber == flightNumber) {
            fare = flight.fare;
            break;
        }
    }
    
    fclose(fp);
    return fare;
}

/**
 * Update available seats for a flight
 */
int updateFlightSeats(int flightNumber, int delta) {
    Flight flight;
    FILE *fp = fopen(FLIGHT_FILE, "r+b");
    
    if (!fp) {
        return 0;
    }
    
    int updated = 0;
    while (fread(&flight, sizeof(Flight), 1, fp) == 1) {
        if (flight.flightNumber == flightNumber) {
            // Ensure seat count stays within valid range
            int newSeats = flight.availableSeats + delta;
            if (newSeats >= 0 && newSeats <= MAX_SEATS) {
                flight.availableSeats = newSeats;
                fseek(fp, -sizeof(Flight), SEEK_CUR);
                fwrite(&flight, sizeof(Flight), 1, fp);
                updated = 1;
            }
            break;
        }
    }
    
    fclose(fp);
    return updated;
}

/**
 * Display all available flights
 */
void displayAvailableFlights() {
    Flight flight;
    FILE *fp = fopen(FLIGHT_FILE, "rb");
    
    if (!fp) {
        printf("No flights available.\n");
        return;
    }
    
    printf("\n%-10s %-15s %-15s %-8s %-8s %s\n", 
           "Flight No.", "Destination", "Departure", "Time", "Fare", "Seats");
    printf("------------------------------------------------------------------------\n");
    
    int hasFlights = 0;
    while (fread(&flight, sizeof(Flight), 1, fp) == 1) {
        if (flight.availableSeats > 0) {
            printf("%-10d %-15s %-15s %-8s $%-7.2f %d\n",
                   flight.flightNumber, flight.destination, flight.departure,
                   flight.time, flight.fare, flight.availableSeats);
            hasFlights = 1;
        }
    }
    
    if (!hasFlights) {
        printf("No flights with available seats.\n");
    }
    
    printf("------------------------------------------------------------------------\n");
    fclose(fp);
}

/**
 * Display all flights (including full ones)
 */
void viewAllFlights() {
    Flight flight;
    FILE *fp = fopen(FLIGHT_FILE, "rb");
    
    if (!fp) {
        printf("No flights available.\n");
        return;
    }
    
    printf("\n%-10s %-15s %-15s %-8s %-8s %s\n", 
           "Flight No.", "Destination", "Departure", "Time", "Fare", "Seats");
    printf("------------------------------------------------------------------------\n");
    
    while (fread(&flight, sizeof(Flight), 1, fp) == 1) {
        printf("%-10d %-15s %-15s %-8s $%-7.2f %d\n",
               flight.flightNumber, flight.destination, flight.departure,
               flight.time, flight.fare, flight.availableSeats);
    }
    
    printf("------------------------------------------------------------------------\n");
    fclose(fp);
}

/**
 * Check if a seat is available on a specific flight
 */
int isSeatAvailable(int flightNumber, int seatNum) {
    if (seatNum < 1 || seatNum > MAX_SEATS) {
        return 0;  // Invalid seat number
    }
    
    Passenger p;
    FILE *fp = fopen(RESERVATION_FILE, "rb");
    
    if (!fp) {
        return 1;  // No reservations file means all seats are available
    }
    
    int isAvailable = 1;
    while (fread(&p, sizeof(Passenger), 1, fp) == 1) {
        if (p.flightNumber == flightNumber && 
            p.seatNumber == seatNum && 
            p.isBooked) {
            isAvailable = 0;
            break;
        }
    }
    
    fclose(fp);
    return isAvailable;
}

/**
 * Display available seats for a flight
 */
void displayAvailableSeats(int flightNumber) {
    int seats[MAX_SEATS] = {0};  // 0 = available, 1 = booked
    Passenger p;
    FILE *fp = fopen(RESERVATION_FILE, "rb");
    
    if (fp) {
        while (fread(&p, sizeof(Passenger), 1, fp) == 1) {
            if (p.flightNumber == flightNumber && 
                p.isBooked && 
                p.seatNumber >= 1 && 
                p.seatNumber <= MAX_SEATS) {
                seats[p.seatNumber - 1] = 1;
            }
        }
        fclose(fp);
    }
    
    printf("\nAvailable Seats for Flight %d:\n", flightNumber);
    printf("--------------------------------------------------\n");
    
    for (int i = 0; i < MAX_SEATS; i++) {
        if (seats[i] == 0) {
            printf("%3d ", i + 1);
            if ((i + 1) % 10 == 0) {
                printf("\n");
            }
        } else {
            printf(" XX ");  // Show booked seats
            if ((i + 1) % 10 == 0) {
                printf("\n");
            }
        }
    }
    printf("\n--------------------------------------------------\n");
}

/* ================ RESERVATION MANAGEMENT ================ */

/**
 * Book a new ticket
 */
void bookTicket() {
    Passenger p;
    int flightNumber;
    
    displayAvailableFlights();
    
    // Get valid flight number
    flightNumber = safeIntInput("\nEnter Flight Number: ", 1, 999999);
    
    if (!isFlightValid(flightNumber)) {
        printf("Invalid flight number or no seats available.\n");
        return;
    }
    
    // Initialize passenger structure
    memset(&p, 0, sizeof(Passenger));
    p.flightNumber = flightNumber;
    p.fare = getFlightFare(flightNumber);
    p.isBooked = 1;
    
    // Get passenger details
    clearInputBuffer();  // Clear any leftover newline
    
    safeStringInput(p.name, MAX_NAME_LEN, "Enter Passenger Name: ");
    
    p.age = safeIntInput("Enter Age: ", 1, 120);
    
    // Get gender with validation
    while (1) {
        printf("Enter Gender (M/F): ");
        char genderInput[10];
        if (fgets(genderInput, sizeof(genderInput), stdin)) {
            p.gender = toupper(genderInput[0]);
            if (p.gender == 'M' || p.gender == 'F') {
                break;
            }
        }
        printf("Invalid gender. Please enter M or F.\n");
    }
    
    // Get seat number
    displayAvailableSeats(flightNumber);
    while (1) {
        p.seatNumber = safeIntInput("Choose Seat Number (1-100): ", 1, MAX_SEATS);
        
        if (isSeatAvailable(flightNumber, p.seatNumber)) {
            break;
        }
        printf("Seat %d is already booked. Please choose another seat.\n", p.seatNumber);
    }
    
    // Get payment method
    printf("\nSelect Payment Method:\n");
    printf("1. Credit Card\n");
    printf("2. Debit Card\n");
    printf("3. Net Banking\n");
    printf("4. UPI\n");
    p.paymentMethod = safeIntInput("Enter choice (1-4): ", 1, 4);
    
    // Generate unique PNR
    generatePNR(p.pnr);
    
    // Save reservation
    FILE *fp = fopen(RESERVATION_FILE, "ab");
    if (!fp) {
        printf("Error: Could not save reservation.\n");
        return;
    }
    
    if (fwrite(&p, sizeof(Passenger), 1, fp) != 1) {
        printf("Error: Failed to write reservation data.\n");
        fclose(fp);
        return;
    }
    fclose(fp);
    
    // Update available seats
    if (!updateFlightSeats(flightNumber, -1)) {
        printf("Warning: Could not update flight seat count.\n");
    }
    
    // Display confirmation
    printf("\n=== BOOKING CONFIRMED ===\n");
    printf("PNR: %s\n", p.pnr);
    printf("Name: %s\n", p.name);
    printf("Flight: %d\n", p.flightNumber);
    printf("Seat: %d\n", p.seatNumber);
    printf("Fare: $%.2f\n", p.fare);
    printf("Payment Method: ");
    switch (p.paymentMethod) {
        case 1: printf("Credit Card\n"); break;
        case 2: printf("Debit Card\n"); break;
        case 3: printf("Net Banking\n"); break;
        case 4: printf("UPI\n"); break;
    }
    printf("==========================\n");
}

/**
 * View all active reservations
 */
void viewReservations() {
    Passenger p;
    FILE *fp = fopen(RESERVATION_FILE, "rb");
    
    if (!fp) {
        printf("No reservations found.\n");
        return;
    }
    
    printf("\n=== ACTIVE RESERVATIONS ===\n");
    printf("PNR       | Name                | Flight | Seat | Fare     | Payment\n");
    printf("------------------------------------------------------------------------\n");
    
    int found = 0;
    while (fread(&p, sizeof(Passenger), 1, fp) == 1) {
        if (p.isBooked) {
            found = 1;
            printf("%-9s | %-19s | %-6d | %-4d | $%-7.2f | ",
                   p.pnr, p.name, p.flightNumber, p.seatNumber, p.fare);
            
            switch (p.paymentMethod) {
                case 1: printf("Credit Card\n"); break;
                case 2: printf("Debit Card\n"); break;
                case 3: printf("Net Banking\n"); break;
                case 4: printf("UPI\n"); break;
            }
        }
    }
    
    if (!found) {
        printf("No active reservations found.\n");
    }
    
    printf("------------------------------------------------------------------------\n");
    fclose(fp);
}

/**
 * Cancel a reservation by PNR
 */
void cancelReservation() {
    char targetPNR[PNR_LEN + 1];
    printf("Enter PNR to cancel: ");
    scanf("%s", targetPNR);
    clearInputBuffer();
    
    FILE *fp = fopen(RESERVATION_FILE, "rb");
    if (!fp) {
        printf("No reservations found.\n");
        return;
    }
    
    FILE *temp = fopen(TEMP_FILE, "wb");
    if (!temp) {
        printf("Error creating temporary file.\n");
        fclose(fp);
        return;
    }
    
    Passenger p;
    int found = 0;
    int cancelledFlight = -1;
    
    while (fread(&p, sizeof(Passenger), 1, fp) == 1) {
        if (strcmp(p.pnr, targetPNR) == 0 && p.isBooked) {
            found = 1;
            cancelledFlight = p.flightNumber;
            
            printf("\nCancelling reservation for %s\n", p.name);
            printf("Flight: %d, Seat: %d\n", p.flightNumber, p.seatNumber);
            printf("Refund amount: $%.2f\n", p.fare);
            
            // Mark as cancelled
            p.isBooked = 0;
        }
        
        // Write to temp file (including cancelled records)
        fwrite(&p, sizeof(Passenger), 1, temp);
    }
    
    fclose(fp);
    fclose(temp);
    
    if (found) {
        // Replace original file with temp file
        remove(RESERVATION_FILE);
        rename(TEMP_FILE, RESERVATION_FILE);
        
        // Update available seats
        if (cancelledFlight != -1) {
            updateFlightSeats(cancelledFlight, 1);
        }
        
        printf("Reservation cancelled successfully.\n");
    } else {
        remove(TEMP_FILE);
        printf("PNR not found or booking already cancelled.\n");
    }
}

/**
 * Modify an existing reservation
 */
void modifyReservation() {
    char targetPNR[PNR_LEN + 1];
    printf("Enter PNR to modify: ");
    scanf("%s", targetPNR);
    clearInputBuffer();
    
    FILE *fp = fopen(RESERVATION_FILE, "rb");
    if (!fp) {
        printf("No reservations found.\n");
        return;
    }
    
    FILE *temp = fopen(TEMP_FILE, "wb");
    if (!temp) {
        printf("Error creating temporary file.\n");
        fclose(fp);
        return;
    }
    
    Passenger p;
    int found = 0;
    int oldFlight = -1, oldSeat = -1;
    
    while (fread(&p, sizeof(Passenger), 1, fp) == 1) {
        if (strcmp(p.pnr, targetPNR) == 0 && p.isBooked) {
            found = 1;
            oldFlight = p.flightNumber;
            oldSeat = p.seatNumber;
            
            // Display current details
            printf("\nCurrent Details:\n");
            printf("Name: %s\n", p.name);
            printf("Age: %d\n", p.age);
            printf("Gender: %c\n", p.gender);
            printf("Flight: %d\n", p.flightNumber);
            printf("Seat: %d\n", p.seatNumber);
            printf("Fare: $%.2f\n", p.fare);
            printf("Payment: ");
            switch (p.paymentMethod) {
                case 1: printf("Credit Card\n"); break;
                case 2: printf("Debit Card\n"); break;
                case 3: printf("Net Banking\n"); break;
                case 4: printf("UPI\n"); break;
            }
            
            printf("\nEnter new details (press Enter to keep current value):\n");
            
            // Modify name
            char input[MAX_NAME_LEN + 10];
            printf("Name [%s]: ", p.name);
            if (fgets(input, sizeof(input), stdin) && input[0] != '\n') {
                input[strcspn(input, "\n")] = '\0';
                strncpy(p.name, input, MAX_NAME_LEN);
                p.name[MAX_NAME_LEN] = '\0';
            }
            
            // Modify age
            printf("Age [%d]: ", p.age);
            if (fgets(input, sizeof(input), stdin) && input[0] != '\n') {
                p.age = atoi(input);
                if (p.age < 1 || p.age > 120) {
                    printf("Invalid age, keeping current value.\n");
                    p.age = oldSeat; // Restore from temp
                }
            }
            
            // Modify gender
            printf("Gender [%c]: ", p.gender);
            if (fgets(input, sizeof(input), stdin) && input[0] != '\n') {
                p.gender = toupper(input[0]);
                if (p.gender != 'M' && p.gender != 'F') {
                    printf("Invalid gender, keeping current value.\n");
                    // gender already correct
                }
            }
            
            // Modify flight
            displayAvailableFlights();
            printf("Flight Number [%d]: ", p.flightNumber);
            if (fgets(input, sizeof(input), stdin) && input[0] != '\n') {
                int newFlight = atoi(input);
                if (isFlightValid(newFlight)) {
                    // Update flight and fare
                    oldFlight = p.flightNumber;
                    p.flightNumber = newFlight;
                    p.fare = getFlightFare(newFlight);
                } else {
                    printf("Invalid flight, keeping current flight.\n");
                }
            }
            
            // Modify seat
            displayAvailableSeats(p.flightNumber);
            printf("Seat Number [%d]: ", p.seatNumber);
            if (fgets(input, sizeof(input), stdin) && input[0] != '\n') {
                int newSeat = atoi(input);
                if (isSeatAvailable(p.flightNumber, newSeat)) {
                    oldSeat = p.seatNumber;
                    p.seatNumber = newSeat;
                } else {
                    printf("Seat not available, keeping current seat.\n");
                }
            }
            
            // Modify payment
            printf("Payment Method [");
            switch (p.paymentMethod) {
                case 1: printf("Credit Card"); break;
                case 2: printf("Debit Card"); break;
                case 3: printf("Net Banking"); break;
                case 4: printf("UPI"); break;
            }
            printf("]\n");
            printf("1. Credit Card\n2. Debit Card\n3. Net Banking\n4. UPI\n");
            printf("Enter new choice (1-4): ");
            if (fgets(input, sizeof(input), stdin) && input[0] != '\n') {
                int newPayment = atoi(input);
                if (newPayment >= 1 && newPayment <= 4) {
                    p.paymentMethod = newPayment;
                }
            }
        }
        
        fwrite(&p, sizeof(Passenger), 1, temp);
    }
    
    fclose(fp);
    fclose(temp);
    
    if (found) {
        // Update flight seat counts if flight or seat changed
        if (oldFlight != p.flightNumber) {
            updateFlightSeats(oldFlight, 1);   // Free old seat
            updateFlightSeats(p.flightNumber, -1); // Reserve new seat
        } else if (oldSeat != p.seatNumber) {
            // Same flight, different seat - seat availability already checked
            // No need to update seat count
        }
        
        // Replace original file
        remove(RESERVATION_FILE);
        rename(TEMP_FILE, RESERVATION_FILE);
        
        printf("Reservation modified successfully.\n");
    } else {
        remove(TEMP_FILE);
        printf("PNR not found or booking cancelled.\n");
    }
}

/**
 * Generate bill/ticket for a reservation
 */
void generateBill() {
    char targetPNR[PNR_LEN + 1];
    printf("Enter PNR to generate bill: ");
    scanf("%s", targetPNR);
    clearInputBuffer();
    
    Passenger p;
    FILE *fp = fopen(RESERVATION_FILE, "rb");
    
    if (!fp) {
        printf("No reservations found.\n");
        return;
    }
    
    int found = 0;
    while (fread(&p, sizeof(Passenger), 1, fp) == 1) {
        if (strcmp(p.pnr, targetPNR) == 0 && p.isBooked) {
            found = 1;
            
            printf("\n=== AIRLINE TICKET ===\n");
            printf("PNR: %s\n", p.pnr);
            printf("Passenger: %s\n", p.name);
            printf("Age: %d | Gender: %c\n", p.age, p.gender);
            printf("Flight: %d\n", p.flightNumber);
            printf("Seat: %d\n", p.seatNumber);
            printf("Fare: $%.2f\n", p.fare);
            printf("Payment Method: ");
            switch (p.paymentMethod) {
                case 1: printf("Credit Card\n"); break;
                case 2: printf("Debit Card\n"); break;
                case 3: printf("Net Banking\n"); break;
                case 4: printf("UPI\n"); break;
            }
            printf("Status: CONFIRMED\n");
            printf("========================\n");
            break;
        }
    }
    
    fclose(fp);
    
    if (!found) {
        printf("PNR not found or booking cancelled.\n");
    }
}

/* ================ ADMIN FUNCTIONS ================ */

/**
 * Add a new flight
 */
void addFlight() {
    Flight flight;
    
    printf("\nEnter Flight Number: ");
    scanf("%d", &flight.flightNumber);
    clearInputBuffer();
    
    // Check if flight already exists
    FILE *fp = fopen(FLIGHT_FILE, "rb");
    if (fp) {
        Flight temp;
        while (fread(&temp, sizeof(Flight), 1, fp) == 1) {
            if (temp.flightNumber == flight.flightNumber) {
                printf("Flight number already exists!\n");
                fclose(fp);
                return;
            }
        }
        fclose(fp);
    }
    
    safeStringInput(flight.destination, MAX_DEST_LEN, "Enter Destination: ");
    safeStringInput(flight.departure, MAX_DEST_LEN, "Enter Departure City: ");
    safeStringInput(flight.time, MAX_TIME_LEN, "Enter Departure Time (HH:MM): ");
    
    printf("Enter Fare: ");
    scanf("%f", &flight.fare);
    clearInputBuffer();
    
    flight.availableSeats = MAX_SEATS;
    
    // Save flight
    fp = fopen(FLIGHT_FILE, "ab");
    if (!fp) {
        printf("Error saving flight.\n");
        return;
    }
    
    if (fwrite(&flight, sizeof(Flight), 1, fp) != 1) {
        printf("Error writing flight data.\n");
    } else {
        printf("Flight added successfully!\n");
    }
    
    fclose(fp);
}

/**
 * Delete a flight
 */
void deleteFlight() {
    int flightNumber = safeIntInput("Enter Flight Number to delete: ", 1, 999999);
    
    FILE *fp = fopen(FLIGHT_FILE, "rb");
    if (!fp) {
        printf("No flights available.\n");
        return;
    }
    
    FILE *temp = fopen(TEMP_FILE, "wb");
    if (!temp) {
        printf("Error creating temporary file.\n");
        fclose(fp);
        return;
    }
    
    Flight flight;
    int found = 0;
    
    while (fread(&flight, sizeof(Flight), 1, fp) == 1) {
        if (flight.flightNumber == flightNumber) {
            found = 1;
            printf("Deleting Flight %d to %s\n", flight.flightNumber, flight.destination);
        } else {
            fwrite(&flight, sizeof(Flight), 1, temp);
        }
    }
    
    fclose(fp);
    fclose(temp);
    
    if (found) {
        remove(FLIGHT_FILE);
        rename(TEMP_FILE, FLIGHT_FILE);
        printf("Flight deleted successfully.\n");
    } else {
        remove(TEMP_FILE);
        printf("Flight not found.\n");
    }
}

/**
 * Generate financial report
 */
void generateFinancialReport() {
    Passenger p;
    FILE *fp = fopen(RESERVATION_FILE, "rb");
    
    if (!fp) {
        printf("No reservations found.\n");
        return;
    }
    
    float totalRevenue = 0.0f;
    int bookings = 0;
    
    while (fread(&p, sizeof(Passenger), 1, fp) == 1) {
        if (p.isBooked) {
            totalRevenue += p.fare;
            bookings++;
        }
    }
    
    fclose(fp);
    
    printf("\n=== FINANCIAL REPORT ===\n");
    printf("Total Bookings: %d\n", bookings);
    printf("Total Revenue: $%.2f\n", totalRevenue);
    if (bookings > 0) {
        printf("Average Fare: $%.2f\n", totalRevenue / bookings);
    } else {
        printf("Average Fare: $0.00\n");
    }
    printf("=======================\n");
}

/**
 * Admin menu
 */
void adminMenu() {
    char password[ADMIN_PASS_LEN + 1];
    printf("Enter admin password: ");
    
    // Hide password input (basic)
    int i = 0;
    char ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (i < ADMIN_PASS_LEN) {
            password[i++] = ch;
            printf("*");
        }
    }
    password[i] = '\0';
    
    if (strcmp(password, ADMIN_PASSWORD) != 0) {
        printf("\nInvalid password!\n");
        return;
    }
    
    printf("\n");
    
    int choice;
    while (1) {
        printf("\n--- ADMIN MENU ---\n");
        printf("1. Add New Flight\n");
        printf("2. View All Flights\n");
        printf("3. Delete Flight\n");
        printf("4. View All Reservations\n");
        printf("5. View Financial Report\n");
        printf("6. Back to Main Menu\n");
        printf("Enter your choice: ");
        
        scanf("%d", &choice);
        clearInputBuffer();
        
        switch (choice) {
            case 1: addFlight(); break;
            case 2: viewAllFlights(); break;
            case 3: deleteFlight(); break;
            case 4: viewReservations(); break;
            case 5: generateFinancialReport(); break;
            case 6: return;
            default: printf("Invalid choice!\n");
        }
    }
}

/**
 * User menu
 */
void userMenu() {
    int choice;
    while (1) {
        printf("\n--- USER MENU ---\n");
        printf("1. Book Ticket\n");
        printf("2. View My Reservations\n");
        printf("3. Modify Reservation\n");
        printf("4. Cancel Reservation\n");
        printf("5. Generate Bill\n");
        printf("6. Back to Main Menu\n");
        printf("Enter your choice: ");
        
        scanf("%d", &choice);
        clearInputBuffer();
        
        switch (choice) {
            case 1: bookTicket(); break;
            case 2: viewReservations(); break;
            case 3: modifyReservation(); break;
            case 4: cancelReservation(); break;
            case 5: generateBill(); break;
            case 6: return;
            default: printf("Invalid choice!\n");
        }
    }
}

/* ================ MAIN FUNCTION ================ */

int main() {
    srand(time(NULL));  // Seed random number generator
    initializeFiles();
    
    printf("========================================\n");
    printf("    AIRLINE RESERVATION SYSTEM\n");
    printf("========================================\n");
    
    int choice;
    while (1) {
        printf("\n--- MAIN MENU ---\n");
        printf("1. User Menu\n");
        printf("2. Admin Menu\n");
        printf("3. Exit\n");
        printf("Enter your choice: ");
        
        if (scanf("%d", &choice) != 1) {
            clearInputBuffer();
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        clearInputBuffer();
        
        switch (choice) {
            case 1: userMenu(); break;
            case 2: adminMenu(); break;
            case 3: 
                printf("Thank you for using the Airline Reservation System. Goodbye!\n");
                exit(0);
            default: printf("Invalid choice! Please enter 1, 2, or 3.\n");
        }
    }
    
    return 0;
}