///////////////////////////////////////////////////////////////////////////////////////////////////
// Company: <IIT ROORKEE>
//
// File: APB_MASTER.v
// File history:
//      <1>: <05/07/2025>: <1st Draft>
//      <2>: <06/07/2025>: <2st Draft>
//
// Description: Advanced Peripheral Bus Master Code to Drive Core I2C.
//
//
// Targeted device: <Family::PolarFireSoC> <Die::MPFS095T> <Package::FCSG325>
// Author: <Ketan Singh>
//
/////////////////////////////////////////////////////////////////////////////////////////////////// 

module ads1115_fsm (
    input clk,
    input rst_n,
    input start,
    input [7:0] prdata,
    output reg [8:0] paddr,
    output reg [7:0] pwdata,
    output reg pwrite,
    output reg penable,
    output reg psel
);


typedef enum logic [6:0] {
    IDLE, SET_CTRL, WAIT_CTRL, LOAD_STA, SEND_STA, CHECK_STA, CTRL_SI_CLEAR_STA, CTRL_SI_CLEAR_NACK_STA,
    SEND_SLA_W, WAIT_SLA_W_ACK, LOAD_STA_SLAW, SEND_STA_SLAW, CHECK_STA_SLAW, CTRL_SI_CLEAR_SLAW_ACK, 
    CTRL_SI_CLEAR_SLAW_NACK,LOAD_REG_PTR, SEND_REG_PTR, LOAD_STA_REG, SEND_STA_REG, CHECK_STA_REG,  
    CTRL_SI_CLEAR_REG_ACK, CTRL_SI_CLEAR_REG_NACK,LOAD_CFG_MSB, SEND_CFG_MSB, LOAD_STA_MSB, SEND_STA_MSB, 
    CHECK_STA_MSB, CTRL_SI_CLEAR_MSB_ACK, CTRL_SI_CLEAR_MSB_NACK,LOAD_CFG_LSB, SEND_CFG_LSB, LOAD_STA_LSB, 
    SEND_STA_LSB, CHECK_STA_LSB, CTRL_SI_CLEAR_LSB_ACK, CTRL_SI_CLEAR_LSB_NACK,LOAD_STOP, SEND_STOP
} state_t;


    state_t state = IDLE;

    // Constants
    localparam SLAVE_ADDR_W  = 8'b10010000; // ADDRESS = 1001000 + 0 (WRITE BIT) = 10010000
    localparam REG_POINTER   = 8'b00000001; // Config register address
    localparam CONFIG_MSB    = 8'b01000010;
    localparam CONFIG_LSB    = 8'b11100011;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state   <= IDLE;
            paddr   <= 9'd0;
            pwdata  <= 8'd0;
            pwrite  <= 1'b0;
            psel    <= 1'b0;
            penable <= 1'b0;
        end else begin
            case (state)
                IDLE: begin
                    psel <= 0;
                    penable <= 0;
                    state <= SET_CTRL;
                end

// START CONDITION SENDING 

                SET_CTRL: begin
                    paddr   <= 9'h000; // CTRL register
                    pwdata  <= 8'b11100000; // cr2=1, ens1=1, sta=1, sto=0, si=0, aa=0, cr1:cr0=0:0 for PCLK/960
                    pwrite  <= 1;
                    psel    <= 1;
                    penable <= 0;
                    state   <= WAIT_CTRL;
                end

                WAIT_CTRL: begin
                    penable <= 1;
                    state <= LOAD_STA;
                end

                LOAD_STA: begin
                    paddr <= 9'h004; // STAT Register
                    pwrite <= 0;
                    psel <= 1;
                    penable <= 0;
                    state <= SEND_STA;
                end
                    
                SEND_STA: begin
                    penable <= 1;
                    state <= CHECK_STA;
                end
                    
                CHECK_STA : begin
                    if(prdata == 8'h08) begin       // START Condition Initiated 
                        paddr <= 9'h000; // CTRL Register
                        pwdata <= 8'b11001000; // si = 1; clearing the signal interrupt,
                        pwrite <= 1;
                        psel <= 1;
                        penable <= 0;
                        state <= CTRL_SI_CLEAR_STA;
                    end 
                    else begin                        // START Condition Not Initiated
                        paddr <= 9'h000;  // CTRL Register
                        pwdata <= 8'b11001000;  // Clearing the interrupt, 
                        pwrite <= 1;
                        psel <= 1;
                        penable <=0;
                        state <= CTRL_SI_CLEAR_NACK_STA;      
                    end
                end
                    
                CTRL_SI_CLEAR_NACK_STA: begin       // In Case of START Condition Not Initiated
                    penable <= 1;
                    state <= SET_CTRL;              // might want to add a delay or completely stop here
                end
                    
                CTRL_SI_CLEAR_STA: begin            // In Case of START Condition Initiated
                    penable <= 1;
                    state <= SEND_SLA_W;
                end

// SLAVE ADDRESS AND WRITE OPERATION BIT SENDING TO THE ADC

                SEND_SLA_W: begin
                    paddr   <= 9'h008; // DATA register
                    pwdata  <= SLAVE_ADDR_W;
                    pwrite  <= 1;
                    psel    <= 1;
                    penable <= 0;
                    state   <= WAIT_SLA_W_ACK;
                end

                WAIT_SLA_W_ACK: begin
                    penable <= 1;
                    state <= LOAD_STA_SLAW;
                end
                    
                LOAD_STA_SLAW: begin
                    paddr <= 9'h004; // STATUS Register
                    pwrite <= 0;
                    psel <= 1;
                    penable <= 0;
                    state <= SEND_STA_SLAW;
                end
                
                SEND_STA_SLAW: begin
                    penable <= 1;
                    state <= CHECK_STA_SLAW;
                end
                
                CHECK_STA_SLAW: begin
                    if (prdata == 8'h18) begin         // ACK recived for Slave Address
                        paddr <= 9'h000; // CTRL Register
                        pwdata <= 8'b11001000; // si = 1; clearing the signal interrupt,
                        pwrite <= 1;
                        psel <= 1;
                        penable <= 0;
                        state <= CTRL_SI_CLEAR_SLAW_ACK;
                    end
                    else if (prdata == 8'h20) begin     // NACK recived for Slave Address
                        paddr <= 9'h000; // CTRL Register
                        pwdata <= 8'b11001000; // si = 1; clearing the signal interrupt,
                        pwrite <= 1;
                        psel <= 1;
                        penable <= 0;
                        state <= CTRL_SI_CLEAR_SLAW_NACK;    // might want to add a else condition to fix unexpected errors
                    end
                end
                
                CTRL_SI_CLEAR_SLAW_NACK: begin         // NACK recived
                    penable <= 1;
                    state <= SEND_SLA_W;               // might want to add a delay or completely stop here
                end

                CTRL_SI_CLEAR_SLAW_ACK: begin        // ACK recived
                    penable <= 1;
                    state <= LOAD_REG_PTR;
                end
                
// Sending the Configuration Register address to the ADC
                
                LOAD_REG_PTR: begin
                    paddr   <= 9'h008;  // DATA Register
                    pwdata  <= REG_POINTER;
                    pwrite  <= 1;
                    psel    <= 1;
                    penable <= 0;
                    state   <= SEND_REG_PTR;
                end

                SEND_REG_PTR: begin
                    penable <= 1;
                    state <= LOAD_STA_REG;
                end
                
                LOAD_STA_REG: begin
                    paddr <= 9'h004; // STATUS Register
                    pwrite <= 0;
                    psel <= 1;
                    penable <= 0;
                    state <= SEND_STA_REG;
                end
                
                SEND_STA_REG: begin
                    penable <= 1;
                    state <= CHECK_STA_REG;
                end
                
                CHECK_STA_REG: begin
                    if (prdata == 8'h28) begin   // ACK recived for Configuration register Address
                        paddr <= 9'h000; // CTRL Register
                        pwdata <= 8'b11001000; // si = 1; clearing the signal interrupt,
                        pwrite <= 1;
                        psel <= 1;
                        penable <= 0;
                        state <= CTRL_SI_CLEAR_REG_ACK;
                        end
                    else if (prdata == 8'h30) begin
                        paddr <= 9'h000; // CTRL Register
                        pwdata <= 8'b11001000; // si = 1; clearing the signal interrupt,
                        pwrite <= 1;
                        psel <= 1;
                        penable <= 0;
                        state <=  CTRL_SI_CLEAR_REG_NACK;     // might want to add a else condition to fix unexpected errors 
                        end
                    end
                    
                CTRL_SI_CLEAR_REG_ACK: begin
                    penable <= 1;
                    state <= LOAD_CFG_MSB;
                end
                
                CTRL_SI_CLEAR_REG_NACK: begin
                    penable <= 1;
                    state <= LOAD_REG_PTR;  // might want to add a delay or completely stop here
                end
                
// Sending MSB for Configuration Register

                LOAD_CFG_MSB: begin
                    paddr   <= 9'h008;  // DATA Register
                    pwdata  <= CONFIG_MSB;
                    pwrite  <= 1;
                    psel    <= 1;
                    penable <= 0;
                    state   <= SEND_CFG_MSB;
                end

                SEND_CFG_MSB: begin
                    penable <= 1;
                    state <= LOAD_STA_MSB;
                end
                
                LOAD_STA_MSB: begin
                    paddr <= 9'h004; // STATUS Register
                    pwrite <= 0;
                    psel <= 1;
                    penable <= 0;
                    state <= SEND_STA_MSB;
                end
                
                SEND_STA_MSB: begin
                    penable <= 1;
                    state <= CHECK_STA_MSB;
                end
                
                CHECK_STA_MSB: begin
                    if (prdata == 8'h28) begin   // ACK recived for Configuration Word MSB
                        paddr <= 9'h000; // CTRL Register
                        pwdata <= 8'b11001000; // si = 1; clearing the signal interrupt,
                        pwrite <= 1;
                        psel <= 1;
                        penable <= 0;
                        state <= CTRL_SI_CLEAR_MSB_ACK;
                    end
                    else if (prdata == 8'h30) begin
                        paddr <= 9'h000; // CTRL Register
                        pwdata <= 8'b11001000; // si = 1; clearing the signal interrupt,
                        pwrite <= 1;
                        psel <= 1;
                        penable <= 0;
                        state <=  CTRL_SI_CLEAR_MSB_NACK;      // might want to add a else condition to fix unexpected errors 
                    end
                end
                    
                CTRL_SI_CLEAR_MSB_ACK: begin
                    penable <= 1;
                    state <= LOAD_CFG_LSB;
                end
                
                CTRL_SI_CLEAR_MSB_NACK: begin
                    penable <= 1;
                    state <= LOAD_CFG_MSB;  // might want to add a delay or completely stop here
                end
                
// Sending LSB for Configuration Register

                LOAD_CFG_LSB: begin
                    paddr   <= 9'h008;  // DATA Register
                    pwdata  <= CONFIG_LSB;
                    pwrite  <= 1;
                    psel    <= 1;
                    penable <= 0;
                    state   <= SEND_CFG_LSB;
                end

                SEND_CFG_LSB: begin
                    penable <= 1;
                    state <= LOAD_STA_LSB;
                end

                LOAD_STA_LSB: begin
                    paddr <= 9'h004; // STATUS Register
                    pwrite <= 0;
                    psel <= 1;
                    penable <= 0;
                    state <= SEND_STA_LSB;
                end
                
                SEND_STA_LSB: begin
                    penable <= 1;
                    state <= CHECK_STA_LSB;
                end
                
                CHECK_STA_LSB: begin
                    if (prdata == 8'h28) begin   // ACK recived for Configuration Word LSB
                        paddr <= 9'h000; // CTRL Register
                        pwdata <= 8'b11001000; // si = 1; clearing the signal interrupt,
                        pwrite <= 1;
                        psel <= 1;
                        penable <= 0;
                        state <= CTRL_SI_CLEAR_LSB_ACK;
                    end
                    else if (prdata == 8'h30) begin
                        paddr <= 9'h000; // CTRL Register
                        pwdata <= 8'b11001000; // si = 1; clearing the signal interrupt,
                        pwrite <= 1;
                        psel <= 1;
                        penable <= 0;
                        state <=  CTRL_SI_CLEAR_LSB_NACK;        // might want to add a else condition to fix unexpected errors 
                    end
                end
                    
                CTRL_SI_CLEAR_LSB_ACK: begin
                    penable <= 1;
                    state <= LOAD_STOP;
                end
                
                CTRL_SI_CLEAR_LSB_NACK: begin
                    penable <= 1;
                    state <= LOAD_CFG_LSB;  // might want to add a delay or completely stop here
                end
                
                LOAD_STOP: begin
                    paddr   <= 9'h000;  // CTRL Register
                    pwdata  <= 8'b11010000; 
                    pwrite  <= 1;
                    psel    <= 1;
                    penable <= 0;
                    state <= SEND_STOP;
                end
                
                SEND_STOP: begin
                    penable <= 1;
                    state <= IDLE;
                end
            endcase
        end
    end
endmodule