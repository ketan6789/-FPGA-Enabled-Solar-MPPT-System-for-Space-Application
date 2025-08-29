module pwm_1hz_50 (
    input clk,          // 2 MHz clock input
    input reset_n,      // Active-low reset input from board
    output reg pwm_out  // PWM output
);

    // Invert active-low reset to active-high
    wire reset = ~reset_n;

    parameter PERIOD = 40;
    parameter HIGH_TIME = PERIOD / 2;

    reg [20:0] counter; // No initializer

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            counter <= 0;        // Reset counter to zero
            pwm_out <= 1'b0;     // Set PWM output to 0 on reset
        end else begin
            if (counter < PERIOD - 1)
                counter <= counter + 1; // Normal counting
            else
                counter <= 0;           // Reset at end of period

            if (counter < HIGH_TIME)
                pwm_out <= 1'b1;        // HIGH for first half
            else
                pwm_out <= 1'b0;        // LOW for second half
        end 
    end
endmodule
