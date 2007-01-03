VERSION 5.00
Begin VB.Form AwkForm 
   Caption         =   "ASE COM AWK"
   ClientHeight    =   7770
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   10335
   LinkTopic       =   "AwkForm"
   ScaleHeight     =   7770
   ScaleWidth      =   10335
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox ConsoleIn 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   9
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   2895
      Left            =   120
      MultiLine       =   -1  'True
      TabIndex        =   4
      Top             =   3600
      Width           =   5055
   End
   Begin VB.TextBox SourceIn 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   9
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   2775
      Left            =   120
      MultiLine       =   -1  'True
      TabIndex        =   3
      Top             =   480
      Width           =   5055
   End
   Begin VB.TextBox SourceOut 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   9
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   2775
      Left            =   5280
      MultiLine       =   -1  'True
      TabIndex        =   2
      Top             =   480
      Width           =   4935
   End
   Begin VB.CommandButton Execute 
      Caption         =   "Execute"
      Height          =   375
      Left            =   7080
      TabIndex        =   1
      Top             =   6840
      Width           =   1215
   End
   Begin VB.TextBox ConsoleOut 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   9
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   2895
      Left            =   5280
      MultiLine       =   -1  'True
      TabIndex        =   0
      Top             =   3600
      Width           =   4935
   End
End
Attribute VB_Name = "AwkForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Dim source_first As Boolean
Public WithEvents Awk As ASELib.Awk
Attribute Awk.VB_VarHelpID = -1

Private Sub Execute_Click()

    source_first = True
    
    ConsoleOut.Text = ""
    SourceOut.Text = ""
    
    Set Awk = New ASELib.Awk
    
    If Awk.Parse() = -1 Then
        MsgBox "ERROR [" + Str(Awk.ErrorLine) + "]" + Awk.ErrorMessage
    Else
        If Awk.Run() = -1 Then
            MsgBox "ERROR [" + Str(Awk.ErrorLine) + "]" + Awk.ErrorMessage
        End If
    End If
    
    Set Awk = Nothing
    
End Sub

Function Awk_OpenSource(ByVal mode As Long) As Long
    Awk_OpenSource = 1
End Function

Function Awk_CloseSource(ByVal mode As Long) As Long
    Awk_CloseSource = 0
End Function

Function Awk_ReadSource(ByVal buf As ASELib.Buffer) As Long
    If source_first Then
        buf.value = SourceIn.Text
        Awk_ReadSource = Len(buf.value)
        source_first = False
    Else
        Awk_ReadSource = 0
    End If
End Function

Function Awk_WriteSource(ByVal buf As ASELib.Buffer) As Long
    Dim value As String
    Dim l As Integer
    
    value = buf.value
    l = Len(value)
    SourceOut.Text = SourceOut.Text + value
    Awk_WriteSource = Len(value)
End Function

Function Awk_OpenExtio(ByVal extio As ASELib.AwkExtio) As Long
    Awk_OpenExtio = -1
    
    Select Case extio.Type
    Case ASELib.AWK_EXTIO_CONSOLE
        If extio.mode = ASELib.AWK_EXTIO_CONSOLE_READ Then
            extio.Handle = New AwkExtioConsole
            With extio.Handle
                .EOF = False
            End With
            Awk_OpenExtio = 1
        ElseIf extio.mode = ASELib.AWK_EXTIO_CONSOLE_WRITE Then
            extio.Handle = New AwkExtioConsole
            With extio.Handle
                .EOF = False
            End With
            Awk_OpenExtio = 1
        End If
        
    Case ASELib.AWK_EXTIO_FILE
        If extio.mode = ASELib.AWK_EXTIO_FILE_READ Then
            extio.Handle = FreeFile
            On Error GoTo ErrorTrap
            Open extio.Name For Input As #extio.Handle
            On Error GoTo 0
            Awk_OpenExtio = 1
        ElseIf extio.mode = ASELib.AWK_EXTIO_FILE_WRITE Then
            extio.Handle = FreeFile
            On Error GoTo ErrorTrap
            Open extio.Name For Output As #extio.Handle
            On Error GoTo 0
            Awk_OpenExtio = 1
        ElseIf extio.mode = ASELib.AWK_EXTIO_FILE_APPEND Then
            extio.Handle = FreeFile
            On Error GoTo ErrorTrap
            Open extio.Name For Append As #extio.Handle
            On Error GoTo 0
            Awk_OpenExtio = 1
        End If
        
    Case ASELib.AWK_EXTIO_PIPE
    Case ASELib.AWK_EXTIO_COPROC
    End Select
    
    Exit Function
    
ErrorTrap:
    Exit Function
End Function

Function Awk_CloseExtio(ByVal extio As ASELib.AwkExtio) As Long
    Awk_CloseExtio = -1
    
    Select Case extio.Type
    Case ASELib.AWK_EXTIO_CONSOLE
        If extio.mode = ASELib.AWK_EXTIO_CONSOLE_READ Or _
           extio.mode = ASELib.AWK_EXTIO_CONSOLE_WRITE Then
            extio.Handle = Nothing
            Awk_CloseExtio = 0
        End If
    Case ASELib.AWK_EXTIO_FILE
        If extio.mode = ASELib.AWK_EXTIO_FILE_READ Or _
           extio.mode = ASELib.AWK_EXTIO_FILE_WRITE Or _
           extio.mode = ASELib.AWK_EXTIO_FILE_APPEND Then
            Close #extio.Handle
            Awk_CloseExtio = 0
        End If
    Case ASELib.AWK_EXTIO_PIPE
    Case ASELib.AWK_EXTIO_COPROC
    End Select
    
End Function

Function Awk_ReadExtio(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Awk_ReadExtio = -1
    
    Select Case extio.Type
    Case ASELib.AWK_EXTIO_CONSOLE
        If extio.mode = ASELib.AWK_EXTIO_CONSOLE_READ Then
            Awk_ReadExtio = ReadExtioConsole(extio, buf)
        End If
        
    Case ASELib.AWK_EXTIO_FILE
        If extio.mode = ASELib.AWK_EXTIO_FILE_READ Then
            Awk_ReadExtio = ReadExtioFile(extio, buf)
        End If
        
    Case ASELib.AWK_EXTIO_PIPE
    Case ASELib.AWK_EXTIO_COPROC
    End Select
    
End Function

Function Awk_WriteExtio(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Awk_WriteExtio = -1
    
    Select Case extio.Type
    Case ASELib.AWK_EXTIO_CONSOLE
        If extio.mode = ASELib.AWK_EXTIO_CONSOLE_WRITE Then
            Awk_WriteExtio = WriteExtioConsole(extio, buf)
        End If
    Case ASELib.AWK_EXTIO_FILE
        If extio.mode = ASELib.AWK_EXTIO_FILE_WRITE Or _
           extio.mode = ASELib.AWK_EXTIO_FILE_APPEND Then
            Awk_WriteExtio = WriteExtioFile(extio, buf)
        End If
    Case ASELib.AWK_EXTIO_PIPE
    Case ASELib.AWK_EXTIO_COPROC
    End Select
End Function

Function ReadExtioConsole(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Dim value As String

    If Not extio.Handle.EOF Then
        value = ConsoleIn.Text
        extio.Handle.EOF = True
        buf.value = value
        ReadExtioConsole = Len(value)
    Else
        ReadExtioConsole = 0
    End If
End Function

Function ReadExtioFile(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Dim value As String
    
    If EOF(extio.Handle) Then
        ReadExtioFile = 0
        Exit Function
    End If
    
    On Error GoTo ErrorTrap
    Line Input #extio.Handle, value
    On Error GoTo 0
    
    value = value + vbCrLf
    
    buf.value = value
    ReadExtioFile = Len(buf.value)
    Exit Function
    
ErrorTrap:
    ReadExtioFile = -1
    Exit Function
End Function
    
Function WriteExtioConsole(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Dim value As String
    'Dim value2 As String
    'Dim i As Long
        
    value = buf.value
    
    'For i = 0 To 5000000
    '    value2 = "abc"
    '    buf.value = "abdkjsdfsafas"
    'Next i
    
    ConsoleOut.Text = ConsoleOut.Text + value
    WriteExtioConsole = Len(value)
End Function

Function WriteExtioFile(ByVal extio As ASELib.AwkExtio, ByVal buf As ASELib.Buffer) As Long
    Dim value As String
    
    WriteExtioFile = -1
    
    value = buf.value
    On Error GoTo ErrorTrap
    Print #extio.Handle, value;
    On Error GoTo 0
    WriteExtioFile = Len(value)
    
    Exit Function
    
ErrorTrap:
    Exit Function
End Function

Private Sub Form_Load()
    'Dim x As ASELib.AwkExtio
    'Dim i As Long
    
    SourceIn.Text = "BEGIN { print 123.12; print 995; print 5432.1; }"
    SourceOut.Text = ""
    ConsoleIn.Text = ""
    ConsoleOut.Text = ""
    
    'Set x = New ASELib.AwkExtio
    'For i = 0 To 50000
    '    x.Handle = New AwkExtioConsole
    'Next i
    'Set x = Nothing
    
    'i = FreeFile
    'On Error GoTo ErrorHandler
    'Open "c:/projects/ase/ase.xdg" For Input As #i
    'On Error GoTo 0
    'MsgBox Input(LOF(i), i)
    'Close #i
    'Exit Sub
    
'ErrorHandler:
'    MsgBox "fuck"
'    Exit Sub
End Sub

