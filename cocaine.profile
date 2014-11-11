{
   "concurrency": 500,
   "startup-timeout": 10.0,
   "termination_timeout": 10.0,
   "idle_timeout": 10.0,
   "queue-limit": 10,
   "grow-threshold": 2,
   "pool-limit": 100,
   "isolate": {
     "args": {
     "spool": "/var/lib/cocaine/"
     },
     "type": "process"
   }
}

