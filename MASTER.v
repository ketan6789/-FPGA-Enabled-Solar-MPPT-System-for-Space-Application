module ads1115_reader (
    input clk,
    input rst,
    input enable,
    output reg [15:0] data_out, // Final 16-bit ADC output
    inout sda,
    inout scl
);

// I2C parameters
parameter IDLE = 0;
parameter START = 1;
parameter SEND_ADDR_W = 2;
parameter ADDR_ACK = 3;
parameter LOAD_CONFIG_ADDR = 4;
parameter WRITE_CONFIG_ADDR =5;
parameter CONFIG_ADDR_ACK = 6;
parameter LOAD_CONFIG_MSB = 7;
parameter WRITE_CONFIG_MSB = 8;
parameter CONFIG_MSB_ACK = 9;
parameter LOAD_CONFIG_LSB = 10;
parameter WRITE_CONFIG_LSB = 11;
parameter CONFIG_LSB_ACK = 12;
parameter RESTART = 13;
parameter SEND_ADDR_R = 14;
parameter READ_MSB = 15;
parameter READ_LSB = 16;
parameter STOP = 17;
parameter DONE = 18;
parameter div_const = 6;


reg [4:0] state = IDLE;
reg [7:0] counter1 = 0;
reg i2c_scl_enable = 0;
reg i2c_clk = 0;

reg [7:0] temp_data;     
reg [7:0] data_msb, data_lsb;
reg wr_enb = 1;
reg sda_out = 1;

reg [7:0] byte_data;
reg [3:0] bit_cnt = 7;

// Constants
wire [6:0] addr_7bit = 7'b1001000; // ADDR = GND ? 0x48
wire [7:0] addr_w = {addr_7bit, 1'b0}; // write address
wire [7:0] addr_r = {addr_7bit, 1'b1}; // read address


always@ (posedge clk)
    begin
        if(counter1 == (div_const /2) - 1)
            begin
                i2c_clk = ~i2c_clk;
                counter1 = 0;
            end
        else
            counter1 = counter1 + 1;
    end

always @(posedge i2c_clk or posedge rst) begin
    if (rst == 1)
        i2c_scl_enable <= 0;
    else if (state == IDLE || state == STOP)
        i2c_scl_enable <= 0;
    else
        i2c_scl_enable <= 1;
end


assign scl = (i2c_scl_enable == 0) ? 1'b1 : i2c_clk;
assign sda = wr_enb ? sda_out : 1'bz;

// FSM for I2C
always @(posedge i2c_clk or posedge rst) begin
    if (rst) begin
        state <= IDLE;
        data_out <= 0;
        data_msb <= 0;
        data_lsb <= 0;
    end else begin
        case (state)
            IDLE: begin
                if (enable) begin
                    state <= START;
                    i2c_scl_enable <= 0;
                    wr_enb <= 0;
                    bit_cnt <= 7;
                end
            end

            START: begin
                wr_enb <= 1; sda_out <= 0; // Start condition
                i2c_scl_enable <= 1;
                state <= SEND_ADDR_W;
                byte_data <= addr_w;
            end

            SEND_ADDR_W: begin
                sda_out <= byte_data[bit_cnt];
                if (bit_cnt == 0) begin
                    state <= ADDR_ACK;
                end else begin
                    bit_cnt <= bit_cnt - 1;
                end
            end
            
            ADDR_ACK : begin
                wr_enb <= 0;
                if (sda == 0) begin
                    state <= IDLE;
                end
            end
            
            LOAD_CONFIG_ADDR : begin 
                bit_cnt <= 7;
                state <= WRITE_CONFIG_MSB;
                wr_enb <= 1;
            end

            WRITE_CONFIG_MSB: begin
                sda_out <= byte_data[bit_cnt];
                if (bit_cnt == 0) begin
                    state <= CONFIG_MSB_ACK;
                end else
                    bit_cnt <= bit_cnt - 1;
            end 

            CONFIG_MSB_ACK : begin
                wr_enb <= 0;
                if (sda == 0) begin
                    state <= IDLE;
                end else begin
                    state <= STOP;
                end
            end
            
            LOAD_CONFIG_LSB : begin
                byte_data <= 8'b11100011;
                bit_cnt <= 7;
                wr_enb <= 1;
                state <= WRITE_CONFIG_LSB;
            end
            
            WRITE_CONFIG_LSB: begin
                sda_out <= byte_data[bit_cnt];
                if (bit_cnt == 0) begin
                    state <= CONFIG_LSB_ACK;
                end else
                    bit_cnt <= bit_cnt - 1;
            end
            
            CONFIG_LSB_ACK : begin
                wr_enb <= 0;
                if (sda == 0) begin
                    state <= STOP;
                end else begin
                    state <= IDLE;
                end
            end

            RESTART: begin
                sda_out <= 0; wr_enb <= 1;
                i2c_scl_enable <= 1;
                state <= SEND_ADDR_R;
            end

            SEND_ADDR_R: begin
                byte_data <= addr_r;
                bit_cnt <= 7;
                state <= READ_MSB;
            end

            READ_MSB: begin
                if (bit_cnt == 0) begin
                    data_msb[bit_cnt] <= sda;
                    bit_cnt <= 7;
                    state <= READ_LSB;
                end else begin
                    data_msb[bit_cnt] <= sda;
                    bit_cnt <= bit_cnt - 1;
                end
            end

            READ_LSB: begin
                if (bit_cnt == 0) begin
                    data_lsb[bit_cnt] <= sda;
                    state <= STOP;
                //end else begin
                    data_lsb[bit_cnt] <= sda;
                    bit_cnt <= bit_cnt - 1;
                end
            end

            STOP: begin
                wr_enb <= 0;
                i2c_scl_enable <= 0;
            end

            DONE: begin
                data_out <= {data_msb, data_lsb};
                state <= IDLE;
            end

        endcase
    end
end
endmodule
