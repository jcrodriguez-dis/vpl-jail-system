#!/usr/bin/env python3
# package:		Part of vpl-jail-system
# copyright:    Copyright (C) 20XX Juan Carlos Rodriguez-del-Pino

"""
VPL Jail System Live Monitor
=============================
Real-time monitoring tool for VPL jail system
Shows live statistics and activity in /var/vpl-jail-system and /jail/home

Usage: sudo python3 vpl_monitor.py
"""

import os
import sys
import time
import json
import psutil
from datetime import datetime
from collections import defaultdict
import glob

# ANSI color codes for terminal output
class Colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

class VPLMonitor:
    def __init__(self):
        self.control_path = "/var/vpl-jail-system"
        self.jail_home_path = "/jail/home"
        self.start_time = time.time()
        
        # Statistics storage
        self.stats = {
            'total_requests': 0,
            'peak_concurrent': 0,
            'total_prisoners_seen': set(),
            'states': defaultdict(int),
            'files_written': 0,
            'files_read': 0,
            'total_cpu_time': 0,
            'total_memory_mb': 0,
        }
        
        # Historical data for tracking changes
        self.previous_prisoners = set()
        self.previous_processes = {}
        
    def check_root(self):
        """Check if running as root"""
        if os.geteuid() != 0:
            print(f"{Colors.FAIL}Error: This script must be run as root!{Colors.ENDC}")
            print("Usage: sudo python3 vpl_monitor.py")
            sys.exit(1)
    
    def clear_screen(self):
        """Clear terminal screen"""
        os.system('clear' if os.name == 'posix' else 'cls')
    
    def get_directory_info(self, path):
        """Get information about a directory"""
        try:
            if not os.path.exists(path):
                return {
                    'exists': False,
                    'files': 0,
                    'size': 0,
                    'subdirs': 0
                }
            
            total_size = 0
            file_count = 0
            dir_count = 0
            
            for root, dirs, files in os.walk(path):
                dir_count += len(dirs)
                for f in files:
                    file_count += 1
                    try:
                        fp = os.path.join(root, f)
                        if os.path.exists(fp):
                            total_size += os.path.getsize(fp)
                    except:
                        pass
            
            return {
                'exists': True,
                'files': file_count,
                'size': total_size,
                'subdirs': dir_count
            }
        except Exception as e:
            return {
                'exists': False,
                'error': str(e),
                'files': 0,
                'size': 0,
                'subdirs': 0
            }
    
    def get_active_prisoners(self):
        """Get list of active prisoner directories"""
        prisoners = []
        try:
            if os.path.exists(self.jail_home_path):
                for entry in os.listdir(self.jail_home_path):
                    if entry.startswith('p'):
                        prisoner_path = os.path.join(self.jail_home_path, entry)
                        if os.path.isdir(prisoner_path):
                            prisoners.append(entry)
        except Exception as e:
            pass
        return prisoners
    
    def get_control_prisoners(self):
        """Get list of prisoner control directories"""
        prisoners = []
        try:
            if os.path.exists(self.control_path):
                for entry in os.listdir(self.control_path):
                    if entry.startswith('p'):
                        control_path = os.path.join(self.control_path, entry)
                        if os.path.isdir(control_path):
                            prisoners.append(entry)
        except Exception as e:
            pass
        return prisoners
    
    def read_prisoner_config(self, prisoner_id):
        """Read prisoner configuration file"""
        config_file = os.path.join(self.control_path, prisoner_id, "config")
        try:
            if os.path.exists(config_file):
                with open(config_file, 'r') as f:
                    config = {}
                    for line in f:
                        line = line.strip()
                        if '=' in line and not line.startswith('#'):
                            key, value = line.split('=', 1)
                            config[key] = value
                    return config
        except:
            pass
        return {}
    
    def get_prisoner_processes(self, prisoner_id):
        """Get processes for a specific prisoner"""
        processes = []
        try:
            for proc in psutil.process_iter(['pid', 'name', 'username', 'cpu_percent', 'memory_info', 'status', 'cmdline', 'cwd']):
                try:
                    # Match by username or working directory containing prisoner ID
                    pinfo = proc.info
                    username = pinfo.get('username', '')
                    cwd = pinfo.get('cwd', '')
                    
                    if prisoner_id in username or prisoner_id in cwd:
                        processes.append({
                            'pid': pinfo['pid'],
                            'name': pinfo['name'],
                            'status': pinfo['status'],
                            'cpu': pinfo['cpu_percent'] or 0,
                            'mem_mb': (pinfo['memory_info'].rss / (1024 * 1024)) if pinfo['memory_info'] else 0,
                            'cmdline': ' '.join(pinfo['cmdline'][:3]) if pinfo['cmdline'] else pinfo['name']
                        })
                except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                    pass
        except:
            pass
        return processes
    
    def get_all_system_info(self):
        """Get all system information in one pass - much faster"""
        info = {
            'vpl_processes': [],
            'prisoner_processes': defaultdict(list),
            'cpu_percent': 0,
            'memory': None,
            'prisoners_home': [],
            'prisoners_control': [],
            'control_info': None,
            'jail_info': None
        }
        
        # System stats
        info['cpu_percent'] = psutil.cpu_percent(interval=0)
        info['memory'] = psutil.virtual_memory()
        
        # Directory info
        info['control_info'] = self.get_directory_info(self.control_path)
        info['jail_info'] = self.get_directory_info(self.jail_home_path)
        
        # Get prisoners
        try:
            if os.path.exists(self.jail_home_path):
                info['prisoners_home'] = [e for e in os.listdir(self.jail_home_path) if e.startswith('p') and os.path.isdir(os.path.join(self.jail_home_path, e))]
        except:
            pass
            
        try:
            if os.path.exists(self.control_path):
                info['prisoners_control'] = [e for e in os.listdir(self.control_path) if e.startswith('p') and os.path.isdir(os.path.join(self.control_path, e))]
        except:
            pass
        
        # Get all processes in one iteration
        try:
            for proc in psutil.process_iter(['pid', 'name', 'username', 'cpu_percent', 'memory_info', 'status', 'cmdline', 'cwd', 'num_threads', 'connections']):
                try:
                    pinfo = proc.info
                    name = pinfo.get('name', '')
                    
                    # VPL server processes
                    if 'vpl-jail-server' in name:
                        info['vpl_processes'].append({
                            'pid': pinfo['pid'],
                            'name': name,
                            'cpu': pinfo['cpu_percent'] or 0,
                            'mem_mb': (pinfo['memory_info'].rss / (1024 * 1024)) if pinfo['memory_info'] else 0,
                            'threads': pinfo.get('num_threads', 0),
                            'connections': len(pinfo.get('connections', []))
                        })
                    
                    # Prisoner processes
                    username = pinfo.get('username', '')
                    cwd = pinfo.get('cwd', '')
                    
                    for prisoner in info['prisoners_control']:
                        if prisoner in username or prisoner in cwd:
                            cmdline = pinfo.get('cmdline', [])
                            cmd_str = ' '.join(cmdline[:4]) if cmdline else name
                            if len(cmd_str) > 60:
                                cmd_str = cmd_str[:57] + '...'
                            
                            info['prisoner_processes'][prisoner].append({
                                'pid': pinfo['pid'],
                                'name': name,
                                'status': pinfo.get('status', 'unknown'),
                                'cpu': pinfo['cpu_percent'] or 0,
                                'mem_mb': (pinfo['memory_info'].rss / (1024 * 1024)) if pinfo['memory_info'] else 0,
                                'cmd': cmd_str
                            })
                            break
                            
                except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                    pass
        except:
            pass
        
        return info
    
    def format_size(self, bytes):
        """Format bytes to human readable size"""
        for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
            if bytes < 1024.0:
                return f"{bytes:.2f} {unit}"
            bytes /= 1024.0
        return f"{bytes:.2f} PB"
    
    def format_uptime(self, seconds):
        """Format seconds to human readable uptime"""
        days = int(seconds // 86400)
        hours = int((seconds % 86400) // 3600)
        minutes = int((seconds % 3600) // 60)
        secs = int(seconds % 60)
        
        if days > 0:
            return f"{days}d {hours}h {minutes}m {secs}s"
        elif hours > 0:
            return f"{hours}h {minutes}m {secs}s"
        elif minutes > 0:
            return f"{minutes}m {secs}s"
        else:
            return f"{secs}s"
    
    def update_statistics(self, prisoners_home, prisoners_control):
        """Update cumulative statistics"""
        current_count = len(prisoners_control)
        if current_count > self.stats['peak_concurrent']:
            self.stats['peak_concurrent'] = current_count
        
        # Track all prisoners we've seen
        for p in prisoners_control:
            self.stats['total_prisoners_seen'].add(p)
        
        # Update states from config files
        for prisoner in prisoners_control:
            config = self.read_prisoner_config(prisoner)
            if 'state' in config:
                self.stats['states'][config['state']] += 1
    
    def display_header(self):
        """Display monitor header"""
        uptime = self.format_uptime(time.time() - self.start_time)
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        print(f"{Colors.BOLD}{Colors.HEADER}{'='*80}{Colors.ENDC}")
        print(f"{Colors.BOLD}{Colors.HEADER}VPL Jail System Live Monitor{Colors.ENDC}")
        print(f"{Colors.OKBLUE}Monitor Running: {uptime} | Current Time: {now}{Colors.ENDC}")
        print(f"{Colors.BOLD}{Colors.HEADER}{'='*80}{Colors.ENDC}\n")
    
    def display_system_info(self, cpu_percent, memory):
        """Display system-wide information"""
        print(f"{Colors.BOLD}{Colors.OKCYAN}[SYSTEM OVERVIEW]{Colors.ENDC}")
        
        print(f"  CPU Usage:    {cpu_percent:6.2f}%")
        print(f"  Memory:       {memory.percent:6.2f}% ({self.format_size(memory.used)} / {self.format_size(memory.total)})")
        
        # Disk usage for key paths
        try:
            jail_disk = psutil.disk_usage('/jail')
            print(f"  /jail Disk:   {jail_disk.percent:6.2f}% ({self.format_size(jail_disk.used)} / {self.format_size(jail_disk.total)})")
        except:
            print(f"  /jail Disk:   Not available")
        
        try:
            var_disk = psutil.disk_usage('/var')
            print(f"  /var Disk:    {var_disk.percent:6.2f}% ({self.format_size(var_disk.used)} / {self.format_size(var_disk.total)})")
        except:
            print(f"  /var Disk:    Not available")
        
        print()
    
    def display_directory_stats(self, control_info, jail_info):
        """Display directory statistics"""
        print(f"{Colors.BOLD}{Colors.OKCYAN}[DIRECTORY STATISTICS]{Colors.ENDC}")
        
        # Control directory
        if control_info['exists']:
            print(f"  {Colors.BOLD}{self.control_path}{Colors.ENDC}")
            print(f"    Status:       {Colors.OKGREEN}Active{Colors.ENDC}")
            print(f"    Subdirs:      {control_info['subdirs']}")
            print(f"    Files:        {control_info['files']}")
            print(f"    Total Size:   {self.format_size(control_info['size'])}")
        else:
            print(f"  {Colors.BOLD}{self.control_path}{Colors.ENDC}")
            print(f"    Status:       {Colors.WARNING}Not Found{Colors.ENDC}")
        
        print()
        
        # Jail home directory
        if jail_info['exists']:
            print(f"  {Colors.BOLD}{self.jail_home_path}{Colors.ENDC}")
            print(f"    Status:       {Colors.OKGREEN}Active{Colors.ENDC}")
            print(f"    Subdirs:      {jail_info['subdirs']}")
            print(f"    Files:        {jail_info['files']}")
            print(f"    Total Size:   {self.format_size(jail_info['size'])}")
        else:
            print(f"  {Colors.BOLD}{self.jail_home_path}{Colors.ENDC}")
            print(f"    Status:       {Colors.WARNING}Not Found{Colors.ENDC}")
        
        print()
    
    def display_active_prisoners(self, prisoners_home, prisoners_control, prisoner_processes):
        """Display active prisoner information with what they're doing"""
        # Update statistics
        self.update_statistics(prisoners_home, prisoners_control)
        
        print(f"{Colors.BOLD}{Colors.OKCYAN}[ACTIVE PRISONERS: {len(prisoners_control)}]{Colors.ENDC}")
        print(f"  In /jail/home: {len(prisoners_home)} | In /var/vpl-jail-system: {len(prisoners_control)}")
        
        # Detect new and removed prisoners
        current_set = set(prisoners_control)
        new_prisoners = current_set - self.previous_prisoners
        removed_prisoners = self.previous_prisoners - current_set
        
        if new_prisoners:
            print(f"  {Colors.OKGREEN}NEW:{Colors.ENDC} {', '.join(sorted(new_prisoners))}")
        if removed_prisoners:
            print(f"  {Colors.WARNING}REMOVED:{Colors.ENDC} {', '.join(sorted(removed_prisoners))}")
        
        self.previous_prisoners = current_set
        print()
        
        # Show detailed info for all active prisoners
        if prisoners_control:
            print(f"{Colors.BOLD}{Colors.OKCYAN}[PRISONER ACTIVITIES - What each prisoner is doing]{Colors.ENDC}")
            
            for prisoner in sorted(prisoners_control):
                config = self.read_prisoner_config(prisoner)
                
                state = config.get('state', 'unknown')
                lang = config.get('lang', 'N/A')
                
                # Color code by state
                state_color = Colors.OKGREEN
                if state in ['compiling', 'running']:
                    state_color = Colors.WARNING
                elif state == 'stopped':
                    state_color = Colors.FAIL
                
                # Get processes for this prisoner
                procs = prisoner_processes.get(prisoner, [])
                proc_count = len(procs)
                total_cpu = sum(p['cpu'] for p in procs)
                total_mem = sum(p['mem_mb'] for p in procs)
                
                # Main prisoner line
                print(f"\n  {Colors.BOLD}{prisoner}{Colors.ENDC} | State: {state_color}{state:12s}{Colors.ENDC} | Lang: {lang:8s} | Procs: {proc_count:2d} | CPU: {total_cpu:5.1f}% | MEM: {total_mem:6.1f}MB")
                
                # Show what processes are running
                if procs:
                    for proc in procs[:5]:  # Max 5 processes per prisoner
                        status_icon = "▶" if proc['status'] == 'running' else "⏸" if proc['status'] == 'sleeping' else "●"
                        print(f"    {status_icon} PID {proc['pid']:6d} {proc['name']:15s} CPU:{proc['cpu']:5.1f}% MEM:{proc['mem_mb']:6.1f}MB | {proc['cmd']}")
                    if len(procs) > 5:
                        print(f"    ... and {len(procs) - 5} more processes")
                else:
                    print(f"    {Colors.WARNING}No active processes detected{Colors.ENDC}")
        else:
            print(f"  {Colors.WARNING}No active prisoners{Colors.ENDC}")
        
        print()
    
    def display_vpl_processes(self, processes):
        """Display VPL jail server processes"""
        print(f"{Colors.BOLD}{Colors.OKCYAN}[VPL JAIL SERVER PROCESSES: {len(processes)}]{Colors.ENDC}")
        
        if processes:
            for proc in processes:
                print(f"  PID {proc['pid']:6d}: {proc['name']:20s} CPU: {proc['cpu']:5.1f}% MEM: {proc['mem_mb']:7.1f}MB Threads: {proc['threads']:3d} Conn: {proc['connections']:3d}")
        else:
            print(f"  {Colors.WARNING}No VPL jail server processes found{Colors.ENDC}")
        
        print()
    
    def display_cumulative_stats(self):
        """Display cumulative statistics since monitoring started"""
        print(f"{Colors.BOLD}{Colors.OKCYAN}[CUMULATIVE STATISTICS]{Colors.ENDC}")
        print(f"  Monitor Uptime:        {self.format_uptime(time.time() - self.start_time)}")
        print(f"  Total Prisoners Seen:  {len(self.stats['total_prisoners_seen'])}")
        print(f"  Peak Concurrent:       {self.stats['peak_concurrent']}")
        
        if self.stats['states']:
            print(f"  {Colors.BOLD}States Observed:{Colors.ENDC}")
            for state, count in sorted(self.stats['states'].items()):
                print(f"    {state:20s}: {count}")
        
        print()
    
    def display_footer(self):
        """Display monitor footer"""
        print(f"{Colors.BOLD}{Colors.HEADER}{'='*80}{Colors.ENDC}")
        print(f"{Colors.OKBLUE}Press Ctrl+C to exit | Updating every 1 second{Colors.ENDC}")
    
    def run(self):
        """Main monitoring loop"""
        self.check_root()
        
        print(f"{Colors.OKGREEN}VPL Jail System Monitor starting...{Colors.ENDC}")
        time.sleep(1)
        
        try:
            while True:
                # Collect ALL data at once - FAST!
                system_info = self.get_all_system_info()
                
                # Clear and display
                self.clear_screen()
                self.display_header()
                self.display_system_info(system_info['cpu_percent'], system_info['memory'])
                self.display_directory_stats(system_info['control_info'], system_info['jail_info'])
                self.display_active_prisoners(
                    system_info['prisoners_home'], 
                    system_info['prisoners_control'],
                    system_info['prisoner_processes']
                )
                self.display_vpl_processes(system_info['vpl_processes'])
                self.display_cumulative_stats()
                self.display_footer()
                
                # Wait 1 second
                time.sleep(1)
                
        except KeyboardInterrupt:
            print(f"\n\n{Colors.OKGREEN}Monitoring stopped by user.{Colors.ENDC}")
            print(f"{Colors.BOLD}Final Statistics:{Colors.ENDC}")
            print(f"  Total monitoring time: {self.format_uptime(time.time() - self.start_time)}")
            print(f"  Total prisoners seen:  {len(self.stats['total_prisoners_seen'])}")
            print(f"  Peak concurrent:       {self.stats['peak_concurrent']}")
            sys.exit(0)
        except Exception as e:
            print(f"\n{Colors.FAIL}Error: {e}{Colors.ENDC}")
            import traceback
            traceback.print_exc()
            sys.exit(1)

def main():
    monitor = VPLMonitor()
    monitor.run()

if __name__ == "__main__":
    main()
