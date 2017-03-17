Issue #25402: In int-to-decimal-string conversion, improve the estimate
of the intermediate memory required, and remove an unnecessarily strict
overflow check. Patch by Serhiy Storchaka.