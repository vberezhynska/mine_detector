Test Server from PowerShell:
$u = [System.Net.Sockets.UdpClient]::new(); $b = [Text.Encoding]::UTF8.GetBytes("TOUCH_EVENT_FROM_PC"); $u.Send($b, $b.Length, "10.42.0.1", 5005); $u.Close()