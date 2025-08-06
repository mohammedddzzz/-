"""
SMS Gateway Manager
Handles multiple SMS sending methods with fallback support
"""

import logging
import time
import smtplib
from email.mime.text import MIMEText
from typing import Optional, Dict, List
from abc import ABC, abstractmethod

import serial
import phonenumbers
from twilio.rest import Client

logger = logging.getLogger(__name__)


class SMSGateway(ABC):
    """Abstract base class for SMS gateways"""
    
    @abstractmethod
    def send_sms(self, recipient: str, message: str) -> bool:
        """Send SMS message"""
        pass
    
    @abstractmethod
    def get_status(self) -> str:
        """Get gateway status"""
        pass
    
    @abstractmethod
    def cleanup(self):
        """Cleanup resources"""
        pass


class GSMModemGateway(SMSGateway):
    """Local GSM modem gateway using AT commands"""
    
    def __init__(self, config: Dict):
        self.port = config.get('port', '/dev/ttyUSB0')
        self.baudrate = config.get('baudrate', 9600)
        self.pin = config.get('pin')
        self.serial_conn = None
        self.is_connected = False
        
        self._connect()
    
    def _connect(self):
        """Connect to GSM modem"""
        try:
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=5
            )
            
            # Test connection
            self._send_at_command("AT")
            
            # Check if PIN is required
            if self.pin:
                self._send_at_command(f"AT+CPIN={self.pin}")
            
            # Set text mode
            self._send_at_command("AT+CMGF=1")
            
            self.is_connected = True
            logger.info(f"Connected to GSM modem on {self.port}")
            
        except Exception as e:
            logger.error(f"Failed to connect to GSM modem: {e}")
            self.is_connected = False
    
    def _send_at_command(self, command: str, wait_time: float = 1) -> str:
        """Send AT command to modem"""
        if not self.serial_conn:
            return ""
            
        self.serial_conn.write(f"{command}\r\n".encode())
        time.sleep(wait_time)
        
        response = ""
        while self.serial_conn.in_waiting:
            response += self.serial_conn.read(self.serial_conn.in_waiting).decode()
        
        return response
    
    def send_sms(self, recipient: str, message: str) -> bool:
        """Send SMS via GSM modem"""
        if not self.is_connected:
            logger.error("GSM modem not connected")
            return False
            
        try:
            # Set recipient
            self._send_at_command(f'AT+CMGS="{recipient}"')
            
            # Send message (Ctrl+Z to send)
            self.serial_conn.write(f"{message}\x1A".encode())
            time.sleep(3)
            
            # Check response
            response = self._send_at_command("", wait_time=0)
            
            if "OK" in response:
                logger.info(f"SMS sent successfully to {recipient}")
                return True
            else:
                logger.error(f"Failed to send SMS: {response}")
                return False
                
        except Exception as e:
            logger.error(f"Error sending SMS via GSM modem: {e}")
            return False
    
    def get_status(self) -> str:
        """Get modem status"""
        if not self.is_connected:
            return "disconnected"
            
        try:
            # Check signal strength
            response = self._send_at_command("AT+CSQ")
            if "OK" in response:
                return "connected"
            else:
                return "error"
        except:
            return "error"
    
    def cleanup(self):
        """Close serial connection"""
        if self.serial_conn:
            self.serial_conn.close()
            logger.info("GSM modem connection closed")


class TwilioGateway(SMSGateway):
    """Twilio SMS gateway"""
    
    def __init__(self, config: Dict):
        self.account_sid = config.get('account_sid')
        self.auth_token = config.get('auth_token')
        self.from_number = config.get('from_number')
        self.client = None
        
        if self.account_sid and self.auth_token:
            try:
                self.client = Client(self.account_sid, self.auth_token)
                logger.info("Twilio client initialized")
            except Exception as e:
                logger.error(f"Failed to initialize Twilio client: {e}")
    
    def send_sms(self, recipient: str, message: str) -> bool:
        """Send SMS via Twilio"""
        if not self.client:
            logger.error("Twilio client not initialized")
            return False
            
        try:
            # Format phone number
            parsed = phonenumbers.parse(recipient, None)
            formatted_number = phonenumbers.format_number(
                parsed, 
                phonenumbers.PhoneNumberFormat.E164
            )
            
            # Send message
            message = self.client.messages.create(
                body=message,
                from_=self.from_number,
                to=formatted_number
            )
            
            logger.info(f"SMS sent via Twilio to {recipient}, SID: {message.sid}")
            return True
            
        except Exception as e:
            logger.error(f"Error sending SMS via Twilio: {e}")
            return False
    
    def get_status(self) -> str:
        """Get Twilio status"""
        if not self.client:
            return "not_configured"
            
        try:
            # Test API connection
            self.client.api.accounts(self.account_sid).fetch()
            return "connected"
        except:
            return "error"
    
    def cleanup(self):
        """No cleanup needed for Twilio"""
        pass


class EmailToSMSGateway(SMSGateway):
    """Email to SMS gateway using carrier gateways"""
    
    def __init__(self, config: Dict):
        self.smtp_server = config.get('smtp_server')
        self.smtp_port = config.get('smtp_port', 587)
        self.username = config.get('username')
        self.password = config.get('password')
        self.carriers = config.get('carriers', {})
    
    def send_sms(self, recipient: str, message: str, carrier: str = None) -> bool:
        """Send SMS via email gateway"""
        if not carrier or carrier not in self.carriers:
            logger.error(f"Unknown carrier: {carrier}")
            return False
            
        try:
            # Extract phone number digits only
            digits = ''.join(filter(str.isdigit, recipient))
            if len(digits) == 11 and digits[0] == '1':
                digits = digits[1:]  # Remove country code for US numbers
            
            # Create email address
            to_email = f"{digits}{self.carriers[carrier]}"
            
            # Create message
            msg = MIMEText(message)
            msg['Subject'] = "Security Alert"
            msg['From'] = self.username
            msg['To'] = to_email
            
            # Send email
            with smtplib.SMTP(self.smtp_server, self.smtp_port) as server:
                server.starttls()
                server.login(self.username, self.password)
                server.send_message(msg)
            
            logger.info(f"SMS sent via email to {to_email}")
            return True
            
        except Exception as e:
            logger.error(f"Error sending SMS via email: {e}")
            return False
    
    def get_status(self) -> str:
        """Get email gateway status"""
        try:
            with smtplib.SMTP(self.smtp_server, self.smtp_port) as server:
                server.starttls()
                server.login(self.username, self.password)
            return "connected"
        except:
            return "error"
    
    def cleanup(self):
        """No cleanup needed for email gateway"""
        pass


class SMSGatewayManager:
    """Manages multiple SMS gateways with fallback support"""
    
    def __init__(self, config: Dict):
        self.gateways = []
        self.email_gateway = None
        
        # Initialize primary gateway
        primary_config = config.get('primary', {})
        if primary_config.get('type') == 'gsm_modem':
            self.gateways.append(GSMModemGateway(primary_config))
        elif primary_config.get('type') == 'twilio':
            self.gateways.append(TwilioGateway(primary_config))
        
        # Initialize fallback gateway
        fallback_config = config.get('fallback', {})
        if fallback_config.get('type') == 'twilio':
            self.gateways.append(TwilioGateway(fallback_config))
        elif fallback_config.get('type') == 'gsm_modem':
            self.gateways.append(GSMModemGateway(fallback_config))
        
        # Initialize email to SMS gateway
        email_config = config.get('email_to_sms', {})
        if email_config.get('type') == 'email':
            self.email_gateway = EmailToSMSGateway(email_config)
    
    def send_sms(self, recipient: str, message: str, carrier: str = None) -> bool:
        """Send SMS using available gateways"""
        # Try primary and fallback gateways
        for gateway in self.gateways:
            try:
                if gateway.send_sms(recipient, message):
                    return True
            except Exception as e:
                logger.error(f"Gateway {type(gateway).__name__} failed: {e}")
        
        # Try email gateway if carrier is specified
        if self.email_gateway and carrier:
            try:
                return self.email_gateway.send_sms(recipient, message, carrier)
            except Exception as e:
                logger.error(f"Email gateway failed: {e}")
        
        logger.error("All SMS gateways failed")
        return False
    
    def get_status(self) -> Dict[str, str]:
        """Get status of all gateways"""
        status = {}
        for i, gateway in enumerate(self.gateways):
            name = f"{type(gateway).__name__}_{i}"
            status[name] = gateway.get_status()
        
        if self.email_gateway:
            status['EmailGateway'] = self.email_gateway.get_status()
        
        return status
    
    def cleanup(self):
        """Cleanup all gateways"""
        for gateway in self.gateways:
            gateway.cleanup()
        
        if self.email_gateway:
            self.email_gateway.cleanup()