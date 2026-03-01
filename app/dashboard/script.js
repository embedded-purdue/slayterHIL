class TestAutomationDashboard {
    constructor() {
        this.runButton = document.querySelector('.run-btn');
        this.logTableBody = document.getElementById('logTableBody');
        this.testItems = document.querySelectorAll('.test-item');
        
        this.initializeEventListeners();
        this.setupWebSocket();
    }

    initializeEventListeners() {
        if (this.runButton) {
            this.runButton.addEventListener('click', () => this.runTest());
        }
        
        // Add click handlers to test items
        this.testItems.forEach(item => {
            item.addEventListener('click', () => {
                this.selectTest(item);
            });
        });
    }

    setupWebSocket() {
        // Replace with your actual WebSocket server URL
        try {
            this.ws = new WebSocket('ws://localhost:8080');

            this.ws.onopen = () => {
                this.addLogEntry('01:15:00', 'info', 'setup', 'Connected to test server');
            };

            this.ws.onmessage = (event) => {
                const message = JSON.parse(event.data);
                this.handleServerMessage(message);
            };

            this.ws.onerror = (error) => {
                console.log('WebSocket error:', error);
            };

            this.ws.onclose = () => {
                console.log('Disconnected from test server');
                // Attempt to reconnect after 5 seconds
                setTimeout(() => this.setupWebSocket(), 5000);
            };
        } catch (error) {
            console.log('WebSocket connection failed, running in standalone mode');
        }
    }

    selectTest(item) {
        // Remove active class from all items
        this.testItems.forEach(i => i.classList.remove('selected'));
        item.classList.add('selected');
    }

    runTest() {
        const timestamp = new Date().toLocaleTimeString('en-US', { hour12: false });
        
        this.addLogEntry(timestamp, 'info', 'setup', 'Test execution started');
        
        // Simulate test execution
        setTimeout(() => {
            this.addLogEntry(timestamp, 'info', 'execute', 'Running test cases...');
        }, 1000);
        
        setTimeout(() => {
            this.addLogEntry(timestamp, 'warn', 'execute', 'Network latency detected');
        }, 2000);
        
        setTimeout(() => {
            this.addLogEntry(timestamp, 'info', 'teardown', 'Test completed successfully');
        }, 3000);
        
        // Send to server if connected
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify({
                command: 'runTest',
                testId: 'search-test'
            }));
        }
    }

    handleServerMessage(message) {
        switch (message.type) {
            case 'log':
                this.addLogEntry(
                    message.timestamp || new Date().toLocaleTimeString('en-US', { hour12: false }),
                    message.level,
                    message.step,
                    message.message
                );
                break;
            case 'status':
                console.log('Status:', message.status);
                break;
        }
    }

    addLogEntry(time, level, step, message) {
        const row = document.createElement('tr');
        row.innerHTML = `
            <td>${time}</td>
            <td><span class="log-level ${level}">${level.toUpperCase()}</span></td>
            <td>${step}</td>
            <td>${message}</td>
        `;
        this.logTableBody.appendChild(row);
        
        // Auto-scroll to bottom
        const logsSection = document.querySelector('.logs-section');
        if (logsSection) {
            logsSection.scrollTop = logsSection.scrollHeight;
        }
    }
}

// Initialize the dashboard when the page loads
document.addEventListener('DOMContentLoaded', () => {
    window.dashboard = new TestAutomationDashboard();
});
